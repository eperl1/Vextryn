import re

with open('gui/compositor/apps/app_browser.hpp', 'r') as f:
    code = f.read()

# Replace all B_ in functions with tab.
code = re.sub(r'\bB_current_page\b', 'tab.current_page', code)
code = re.sub(r'\bB_url_len\b', 'tab.url_len', code)
code = re.sub(r'\bB_url_buffer\b', 'tab.url_buffer', code)
code = re.sub(r'\bB_url_input\b', 'tab.url_input', code)
code = re.sub(r'\bB_history_count\b', 'tab.history_count', code)
code = re.sub(r'\bB_history_idx\b', 'tab.history_idx', code)
code = re.sub(r'\bB_browser_history\b', 'tab.history', code)
code = re.sub(r'\bB_search_buffer\b', 'tab.search_buffer', code)
code = re.sub(r'\bB_search_len\b', 'tab.search_len', code)
code = re.sub(r'\bB_search_input\b', 'tab.search_input', code)

with open('gui/compositor/apps/app_browser.hpp', 'w') as f:
    f.write(code)
