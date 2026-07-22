import gi
import sys
import os
import json
import urllib.parse
import time

gi.require_version('Gtk', '3.0')
gi.require_version('WebKit2', '4.1')
from gi.repository import Gtk, Gdk, WebKit2, GLib

DATA_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "data")
DL_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "downloads")
os.makedirs(DATA_DIR, exist_ok=True)
os.makedirs(DL_DIR, exist_ok=True)
DATA_FILE = os.path.join(DATA_DIR, "browser_data.json")

def load_data():
    if os.path.exists(DATA_FILE):
        try:
            with open(DATA_FILE, "r") as f:
                return json.load(f)
        except Exception:
            pass
    return {"history": [], "bookmarks": []}

def save_data(data):
    tmp = DATA_FILE + ".tmp"
    with open(tmp, "w") as f:
        json.dump(data, f)
    os.rename(tmp, DATA_FILE)

browser_data = load_data()

def add_history(title, url):
    if not url or url.startswith("about:") or url.startswith("vextryn://"): return
    if title == url or not title: title = "Untitled"
    browser_data["history"].insert(0, {"title": title, "url": url, "time": time.time()})
    browser_data["history"] = browser_data["history"][:500]
    save_data(browser_data)

def toggle_bookmark(title, url):
    if not url or url.startswith("about:") or url.startswith("vextryn://"): return False
    for i, b in enumerate(browser_data["bookmarks"]):
        if b["url"] == url:
            browser_data["bookmarks"].pop(i)
            save_data(browser_data)
            return False
    browser_data["bookmarks"].append({"title": title if title else url, "url": url})
    save_data(browser_data)
    return True

def is_bookmarked(url):
    if not url: return False
    return any(b["url"] == url for b in browser_data["bookmarks"])


class BrowserTab(Gtk.VBox):
    def __init__(self, browser_window, url=None):
        super().__init__()
        self.window = browser_window
        
        self.webview = WebKit2.WebView()
        self.webview.connect("load-changed", self.on_load_changed)
        self.webview.connect("notify::estimated-load-progress", self.on_progress)
        self.webview.connect("notify::title", self.on_title_changed)
        self.webview.connect("load-failed-with-tls-errors", self.on_tls_error)
        self.webview.connect("load-failed", self.on_load_failed)
        self.webview.connect("decide-policy", self.on_decide_policy)
        
        scroll = Gtk.ScrolledWindow()
        scroll.add(self.webview)
        self.pack_start(scroll, True, True, 0)
        
        if url:
            self.load_url(url)
        else:
            self.load_new_tab_page()
            
    def load_url(self, url):
        if url == "vextryn://history":
            self.load_history_page()
        elif url == "about:blank" or not url:
            self.load_new_tab_page()
        else:
            self.webview.load_uri(url)
            
    def load_new_tab_page(self):
        html = """
        <style>
            body{font-family:sans-serif; text-align:center; padding:50px; background:#f9f9f9;}
            .links a { display:inline-block; margin:10px; padding:10px 20px; background:#e0e0e0; text-decoration:none; color:#333; border-radius:5px;}
        </style>
        <h2>VextrynAir Browser Preview</h2>
        <p style="color:red; font-weight:bold;">Linux host prototype; not running inside VextrynAir OS</p>
        <div class="links">
            <a href="https://example.com">Example</a>
            <a href="https://www.wikipedia.org">Wikipedia</a>
            <a href="https://news.ycombinator.com">Hacker News</a>
            <a href="vextryn://history">History</a>
        </div>
        """
        self.webview.load_html(html, "about:blank")
        global screenshot_file
        if screenshot_file:
            GLib.timeout_add(1500, self.window.take_screenshot)
        
    def load_history_page(self):
        html = "<style>body{font-family:sans-serif; padding:20px;}</style><h2>History</h2><ul>"
        for h in browser_data["history"]:
            t = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(h["time"]))
            html += f'<li>[{t}] <a href="{h["url"]}">{h["title"]}</a></li>'
        html += "</ul>"
        self.webview.load_html(html, "vextryn://history")
        global screenshot_file
        if screenshot_file:
            GLib.timeout_add(1500, self.window.take_screenshot)

    def on_decide_policy(self, webview, decision, decision_type):
        if decision_type == WebKit2.PolicyDecisionType.NAVIGATION_ACTION:
            req = decision.get_request()
            uri = req.get_uri()
            if uri and uri.startswith("vextryn://history"):
                decision.ignore()
                self.load_history_page()
                return True
        return False
            
    def on_load_changed(self, webview, load_event):
        if load_event == WebKit2.LoadEvent.FINISHED:
            uri = self.webview.get_uri()
            title = self.webview.get_title()
            add_history(title, uri)
        if self.window.get_current_tab() == self:
            self.window.update_ui()
            
    def on_progress(self, webview, param):
        if self.window.get_current_tab() == self:
            self.window.update_progress()
            
    def on_title_changed(self, webview, param):
        self.window.update_tab_title(self)
        if self.window.get_current_tab() == self:
            self.window.update_ui()
        
    def on_tls_error(self, webview, failing_uri, cert, errors):
        html = f"<h1>TLS Certificate Error</h1><p>Failed to securely load {failing_uri}</p>"
        webview.load_alternate_html(html, failing_uri, None)
        return True
        
    def on_load_failed(self, webview, load_event, failing_uri, error):
        # Prevent catching TLS error twice if handled
        if error.domain == "WebKitNetworkError" and error.code == 70: # TLS error code
            return False
        html = f"<h1>Load Error</h1><p>Failed to load {failing_uri}</p><p>{error.message}</p>"
        webview.load_alternate_html(html, failing_uri, None)
        return True

class VextrynBrowser(Gtk.Window):
    def __init__(self):
        super().__init__(title="VextrynAir Browser Preview")
        self.set_default_size(1024, 768)
        
        # Connect downloads
        context = WebKit2.WebContext.get_default()
        context.connect("download-started", self.on_download_started)
        
        self.dl_status = Gtk.Label(label="")
        
        vbox = Gtk.VBox()
        self.add(vbox)
        
        # Toolbar
        toolbar = Gtk.Toolbar()
        self.back_btn = Gtk.ToolButton(label="Back")
        self.forward_btn = Gtk.ToolButton(label="Forward")
        self.reload_btn = Gtk.ToolButton(label="Reload")
        
        self.back_btn.connect("clicked", self.on_back)
        self.forward_btn.connect("clicked", self.on_forward)
        self.reload_btn.connect("clicked", self.on_reload)
        
        toolbar.insert(self.back_btn, -1)
        toolbar.insert(self.forward_btn, -1)
        toolbar.insert(self.reload_btn, -1)
        
        self.url_entry = Gtk.Entry()
        self.url_entry.connect("activate", self.on_url_enter)
        url_item = Gtk.ToolItem()
        url_item.set_expand(True)
        url_item.add(self.url_entry)
        toolbar.insert(url_item, -1)
        
        self.star_btn = Gtk.ToolButton(label="☆")
        self.star_btn.connect("clicked", self.on_star_clicked)
        toolbar.insert(self.star_btn, -1)
        
        vbox.pack_start(toolbar, False, False, 0)
        
        # Progress bar
        self.progress = Gtk.ProgressBar()
        vbox.pack_start(self.progress, False, False, 0)
        
        # Notebook (Tabs)
        self.notebook = Gtk.Notebook()
        self.notebook.connect("switch-page", self.on_tab_changed)
        vbox.pack_start(self.notebook, True, True, 0)
        
        vbox.pack_start(self.dl_status, False, False, 0)
        
        # New tab button
        new_tab_btn = Gtk.Button(label="+")
        new_tab_btn.connect("clicked", lambda x: self.new_tab())
        self.notebook.set_action_widget(new_tab_btn, Gtk.PackType.END)
        new_tab_btn.show()
        
        # Accelerators
        accel = Gtk.AccelGroup()
        self.add_accel_group(accel)
        
        key, mod = Gtk.accelerator_parse("<Control>t")
        accel.connect(key, mod, Gtk.AccelFlags.VISIBLE, lambda *a: self.new_tab())
        key, mod = Gtk.accelerator_parse("<Control>w")
        accel.connect(key, mod, Gtk.AccelFlags.VISIBLE, lambda *a: self.close_current_tab())
        key, mod = Gtk.accelerator_parse("<Control>l")
        accel.connect(key, mod, Gtk.AccelFlags.VISIBLE, lambda *a: self.url_entry.grab_focus())
        key, mod = Gtk.accelerator_parse("<Alt>Left")
        accel.connect(key, mod, Gtk.AccelFlags.VISIBLE, self.on_back)
        key, mod = Gtk.accelerator_parse("<Alt>Right")
        accel.connect(key, mod, Gtk.AccelFlags.VISIBLE, self.on_forward)
        
        self.new_tab()
        
    def on_download_started(self, context, download):
        dest_uri = download.get_request().get_uri()
        filename = os.path.basename(urllib.parse.urlparse(dest_uri).path)
        if not filename: filename = "download.bin"
        filename = "".join(c for c in filename if c.isalnum() or c in ".-_")
        dest_path = os.path.join(DL_DIR, filename)
        
        download.set_destination("file://" + dest_path)
        self.dl_status.set_text(f"Downloading: {filename}...")
        
        def on_finished(dl):
            self.dl_status.set_text(f"Download complete: {filename}")
        def on_failed(dl, err):
            self.dl_status.set_text(f"Download failed: {filename}")
            
        download.connect("finished", on_finished)
        download.connect("failed", on_failed)
        
    def new_tab(self, url=""):
        tab = BrowserTab(self, url)
        
        hbox = Gtk.HBox()
        label = Gtk.Label(label="New Tab")
        close_btn = Gtk.Button(label="X")
        close_btn.set_relief(Gtk.ReliefStyle.NONE)
        close_btn.connect("clicked", self.on_close_tab_btn, tab)
        
        hbox.pack_start(label, True, True, 0)
        hbox.pack_start(close_btn, False, False, 0)
        hbox.show_all()
        
        tab.label_widget = label
        tab.show_all()
        
        self.notebook.append_page(tab, hbox)
        self.notebook.set_current_page(self.notebook.get_n_pages() - 1)
            
    def close_current_tab(self):
        tab = self.get_current_tab()
        if tab:
            self.on_close_tab_btn(None, tab)
            
    def on_close_tab_btn(self, button, tab):
        page_num = self.notebook.page_num(tab)
        self.notebook.remove_page(page_num)
        if self.notebook.get_n_pages() == 0:
            self.new_tab()
            
    def get_current_tab(self):
        page_num = self.notebook.get_current_page()
        if page_num >= 0:
            return self.notebook.get_nth_page(page_num)
        return None
        
    def on_tab_changed(self, notebook, page, page_num):
        GLib.idle_add(self.update_ui)
        
    def update_ui(self):
        tab = self.get_current_tab()
        if not tab: return
        self.back_btn.set_sensitive(tab.webview.can_go_back())
        self.forward_btn.set_sensitive(tab.webview.can_go_forward())
        
        uri = tab.webview.get_uri()
        if uri and not uri.startswith("about:"):
            self.url_entry.set_text(uri)
        else:
            self.url_entry.set_text("")
            
        if is_bookmarked(uri):
            self.star_btn.set_label("★")
        else:
            self.star_btn.set_label("☆")
            
    def update_progress(self):
        tab = self.get_current_tab()
        if not tab: return
        prog = tab.webview.get_estimated_load_progress()
        self.progress.set_fraction(prog)
        if prog == 1.0:
            self.progress.hide()
            global screenshot_file
            if screenshot_file:
                GLib.timeout_add(1500, self.take_screenshot)
        else:
            self.progress.show()
            
    def take_screenshot(self):
        global screenshot_file
        window = self.get_window()
        width = window.get_width()
        height = window.get_height()
        pb = Gdk.pixbuf_get_from_window(window, 0, 0, width, height)
        pb.savev(screenshot_file, "png", [], [])
        print(f"Screenshot saved to {screenshot_file}")
        screenshot_file = None
        Gtk.main_quit()
        return False
            
    def update_tab_title(self, tab):
        title = tab.webview.get_title()
        if title:
            tab.label_widget.set_text(title[:20])
            self.set_title(f"{title} - VextrynAir Browser Preview")
            
    def on_star_clicked(self, btn):
        tab = self.get_current_tab()
        if not tab: return
        uri = tab.webview.get_uri()
        title = tab.webview.get_title()
        if toggle_bookmark(title, uri):
            self.star_btn.set_label("★")
        else:
            self.star_btn.set_label("☆")
            
    def on_back(self, *args):
        tab = self.get_current_tab()
        if tab: tab.webview.go_back()
        return True
        
    def on_forward(self, *args):
        tab = self.get_current_tab()
        if tab: tab.webview.go_forward()
        return True
        
    def on_reload(self, *args):
        tab = self.get_current_tab()
        if tab: tab.webview.reload()
        
    def on_url_enter(self, entry):
        tab = self.get_current_tab()
        if not tab: return
        url = entry.get_text().strip()
        if not url: return
        
        if " " in url or ("." not in url and not url.startswith("vextryn://") and not url.startswith("about:")):
            url = "https://www.google.com/search?q=" + urllib.parse.quote(url)
        elif not url.startswith("http://") and not url.startswith("https://") and not url.startswith("vextryn://") and not url.startswith("about:"):
            url = "https://" + url
            
        tab.load_url(url)

if __name__ == "__main__":
    screenshot_file = None
    args = sys.argv[1:]
    if "--screenshot" in args:
        idx = args.index("--screenshot")
        screenshot_file = args[idx+1]
        args.pop(idx)
        args.pop(idx)
        
    win = VextrynBrowser()
    win.connect("destroy", Gtk.main_quit)
    win.show_all()
    
    if len(args) > 0:
        win.get_current_tab().load_url(args[0])
        
    Gtk.main()
