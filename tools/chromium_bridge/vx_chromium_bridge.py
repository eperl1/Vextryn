import json
import sys
import time
import urllib.parse
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

try:
    from playwright.sync_api import sync_playwright
except Exception:
    sync_playwright = None


HOST = "127.0.0.1"
PORT = 8081


def html_escape(text: str) -> str:
    return (
        text.replace("&", "&amp;")
        .replace("<", "&lt;")
        .replace(">", "&gt;")
        .replace('"', "&quot;")
    )


class ChromiumBridge:
    def __init__(self):
        self.play = None
        self.browser = None

    def ensure_browser(self):
        if self.browser is not None:
            return
        if sync_playwright is None:
            raise RuntimeError("Playwright is not installed")
        self.play = sync_playwright().start()
        self.browser = self.play.chromium.launch(headless=True)

    def render(self, url: str) -> str:
        self.ensure_browser()
        page = self.browser.new_page(viewport={"width": 1280, "height": 900})
        try:
            page.goto(url, wait_until="domcontentloaded", timeout=20000)
            try:
                page.wait_for_load_state("networkidle", timeout=6000)
            except Exception:
                pass
            title = page.title() or url
            page_html = page.eval_on_selector("body", "el => el.innerHTML") or ""
            meta = (
                "<div>"
                "<p>Rendered by host Chromium bridge.</p>"
                f"<p>Source: <a href=\"{html_escape(url)}\">{html_escape(url)}</a></p>"
                "</div>"
            )
            return (
                "<html><body>"
                f"<h1>{html_escape(title)}</h1>"
                f"{meta}"
                "<hr/>"
                f"{page_html}"
                "</body></html>"
            )
        finally:
            page.close()

    def close(self):
        if self.browser is not None:
            self.browser.close()
            self.browser = None
        if self.play is not None:
            self.play.stop()
            self.play = None


BRIDGE = ChromiumBridge()


class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        parsed = urllib.parse.urlparse(self.path)
        if parsed.path == "/health":
            payload = json.dumps({"ok": True, "playwright": sync_playwright is not None}).encode("utf-8")
            self.send_response(200)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.send_header("Content-Length", str(len(payload)))
            self.end_headers()
            self.wfile.write(payload)
            return

        if parsed.path != "/render":
            self.send_error(404, "Not Found")
            return

        query = urllib.parse.parse_qs(parsed.query)
        url = query.get("url", [""])[0]
        if not url:
            self.send_error(400, "Missing url")
            return

        try:
            html = BRIDGE.render(url)
            body = html.encode("utf-8", errors="replace")
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
        except Exception as exc:
            error_html = (
                "<html><body><h1>Bridge Render Error</h1>"
                f"<p>{html_escape(str(exc))}</p>"
                f"<p>URL: {html_escape(url)}</p>"
                "</body></html>"
            ).encode("utf-8", errors="replace")
            self.send_response(500)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", str(len(error_html)))
            self.end_headers()
            self.wfile.write(error_html)

    def log_message(self, fmt, *args):
        stamp = time.strftime("%H:%M:%S")
        sys.stdout.write(f"[{stamp}] " + (fmt % args) + "\n")
        sys.stdout.flush()


def main():
    server = ThreadingHTTPServer((HOST, PORT), Handler)
    print(f"Chromium bridge listening on http://{HOST}:{PORT}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()
        BRIDGE.close()


if __name__ == "__main__":
    main()
