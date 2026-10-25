# -*- coding: utf-8 -*-
"""Read-only, repository-confined HTTP documentation and source browser."""
from __future__ import print_function, unicode_literals
import argparse
import io
import re
import os
import threading
import webbrowser

try:
    from http.server import BaseHTTPRequestHandler, HTTPServer
    from urllib.parse import quote, unquote, urlsplit
    from html.parser import HTMLParser
    from html import escape, unescape
except ImportError:
    from BaseHTTPServer import BaseHTTPRequestHandler, HTTPServer
    from urllib import quote, unquote
    from urlparse import urlsplit
    from HTMLParser import HTMLParser
    from cgi import escape

    unescape = HTMLParser().unescape

# Default serving boundary is this component, even when renamed or detached.
PROJECT_ROOT = os.path.realpath(os.path.join(os.path.dirname(__file__), os.pardir))
ROOT = PROJECT_ROOT
HOME = "README.md"
SOURCE_EXTENSIONS = (
    ".c",
    ".h",
    ".cpp",
    ".hpp",
    ".mm",
    ".m",
    ".py",
    ".java",
    ".as",
    ".atg",
    ".txt",
    ".defs",
    ".cmake",
    ".json",
    ".sh",
    ".css",
    ".js",
)
IMAGE_TYPES = {
    ".png": "image/png",
    ".jpg": "image/jpeg",
    ".jpeg": "image/jpeg",
    ".gif": "image/gif",
    ".webp": "image/webp",
    ".ico": "image/x-icon",
}


def url_path(path):
    """Encode repository paths as UTF-8 URLs on both interpreter families."""
    return quote(path.encode("utf-8"), safe=b"/")


def safe_file(root, path):
    """Resolve one visible repository file, rejecting traversal and escaping symlinks."""
    parts = path.replace("\\", "/").split("/")
    if any(part.startswith(".") or ":" in part for part in parts if part):
        raise ValueError("Hidden paths and traversal are not served")
    resolved = os.path.realpath(os.path.join(root, *parts))
    resolved_parts = os.path.relpath(resolved, root).replace(os.sep, "/").split("/")
    if any(part.startswith(".") for part in resolved_parts):
        raise ValueError("Hidden symlink destinations are not served")
    if not resolved.startswith(root + os.sep) or not os.path.isfile(resolved):
        raise ValueError("File is outside the repository or does not exist")
    return resolved


class SafeMarkdown(HTMLParser):
    """Keep useful Markdown HTML while removing executable tags and URL attributes."""

    tags = set(
        (
            "p div span small em strong b i a h1 h2 h3 h4 h5 h6 ul ol li "
            "blockquote pre code table thead tbody tr th td hr br img del "
            "details summary sup sub input"
        ).split()
    )
    void = set(("hr", "br", "img", "input"))

    def __init__(self):
        """Create a fresh output buffer for one rendered document."""
        HTMLParser.__init__(self)
        self.output = []

    def handle_starttag(self, tag, attrs):
        """Allow only formatting, anchors and non-executable links/images."""
        if tag not in self.tags:
            return
        clean = []
        for key, value in attrs:
            if value is None:
                continue
            if key in ("href", "src"):
                scheme = urlsplit(value.strip()).scheme.lower()
                if scheme not in ("", "http", "https", "mailto"):
                    continue
                if key == "src" and scheme:
                    continue
            elif key not in ("id", "name", "title", "alt", "class", "colspan", "rowspan"):
                continue
            clean.append(' %s="%s"' % (key, escape(value, quote=True)))
        self.output.append("<%s%s>" % (tag, "".join(clean)))

    def handle_startendtag(self, tag, attrs):
        """Normalize self-closing Markdown HTML elements."""
        self.handle_starttag(tag, attrs)
        if tag not in self.void:
            self.handle_endtag(tag)

    def handle_endtag(self, tag):
        """Preserve closing tags only for permitted non-void elements."""
        if tag in self.tags and tag not in self.void:
            self.output.append("</%s>" % tag)

    def handle_data(self, data):
        """Escape literal document text regardless of the original HTML context."""
        self.output.append(escape(data))

    def handle_entityref(self, name):
        """Preserve existing named HTML entities without double escaping."""
        self.output.append("&%s;" % name)

    def handle_charref(self, name):
        """Preserve numeric HTML entities in document text."""
        self.output.append("&#%s;" % name)


def render_markdown(text):
    """Render fenced code, tables and headings, then sanitize embedded HTML."""
    import markdown2

    class RepositoryMarkdown(markdown2.Markdown):
        """Preserve Unicode and underscore heading anchors used by repository links."""

        def __init__(self, *args, **kwargs):
            """Track actual emitted IDs within this one document conversion."""
            markdown2.Markdown.__init__(self, *args, **kwargs)
            self.heading_ids = set()

        def header_id_from_text(self, heading, prefix, level):
            """Generate GitHub-style heading IDs, including duplicate suffixes."""
            label = unescape(re.sub("<[^>]*>", "", heading)).lower()
            label = re.sub(r"[^\w\- ]", "", label, flags=re.UNICODE).replace(" ", "-")
            count = self._count_from_header_id[label]
            candidate = label if not count else label + "-" + str(count)
            # A literal heading such as API-1 can already own the next suffix.
            while candidate in self.heading_ids:
                count += 1
                candidate = label + "-" + str(count)
            self._count_from_header_id[label] = count + 1
            self.heading_ids.add(candidate)
            return candidate

    rendered = RepositoryMarkdown(
        extras=["fenced-code-blocks", "tables", "strike", "header-ids"]
    ).convert(text)
    parser = SafeMarkdown()
    parser.feed(rendered)
    parser.close()
    return "".join(parser.output)


def render_source(text, filename):
    """Highlight source and give every physical line a stable #L<number> anchor."""
    from pygments import highlight
    from pygments.lexers import get_lexer_for_filename, TextLexer
    from pygments.formatters import HtmlFormatter
    from pygments.util import ClassNotFound

    try:
        lexer = get_lexer_for_filename(filename, stripnl=False, ensurenl=False)
    except ClassNotFound:
        lexer = TextLexer(stripnl=False, ensurenl=False)
    colored = highlight(text, lexer, HtmlFormatter(nowrap=True)).splitlines()
    lines = []
    for number, code in enumerate(colored, 1):
        lines.append(
            '<span class="source-line" id="L%d"><a class="line-number" '
            'href="#L%d">%d</a>%s</span>' % (number, number, number, code)
        )
    return '<pre class="source"><code>%s</code></pre>' % "\n".join(lines)


def navigation(root, component=""):
    """Index Markdown guides and example READMEs in alphabetic folder order."""
    groups = [
        ("FlashVM", component),
        ("Documentation", os.path.join(component, "doc")),
        ("Examples", os.path.join(component, "examples")),
        ("Desktop frontend", os.path.join(component, "frontend")),
    ]
    if component:
        groups.insert(0, ("Repository", ""))
        # Workspace-only navigation: detached VM browsing never inspects siblings.
        for title, folder in [("Compilers", "compiler"), ("Linker", "linker"), ("FIR", "fir")]:
            if os.path.isdir(os.path.join(root, folder)):
                groups.append((title, folder))
    blocks = []
    for title, folder in groups:
        files = []
        base = os.path.join(root, folder)
        # FIR may itself be the selected component; still index every version.
        if folder in ("", component) and title != "FIR":
            files = [
                os.path.join(folder, name).replace(os.sep, "/")
                for name in os.listdir(base)
                if name.lower().endswith(".md")
            ]
        else:
            for current, directories, names in os.walk(base, followlinks=False):
                directories[:] = sorted(d for d in directories if not d.startswith("."))
                for name in names:
                    if name.lower().endswith(".md"):
                        files.append(
                            os.path.relpath(os.path.join(current, name), root).replace(os.sep, "/")
                        )
        links = [
            '<li><a href="/view/%s">%s</a></li>' % (url_path(path), escape(path))
            for path in sorted(files, key=lambda p: p.lower())
        ]
        blocks.append(
            "<details open><summary>%s</summary><ul>%s</ul></details>" % (title, "".join(links))
        )
    return "".join(blocks)


def page(path, content, nav):
    """Wrap UTF-8 content in a responsive, searchable documentation shell."""
    return (
        '<!doctype html><html lang="en"><head><meta charset="utf-8">'
        '<meta name="viewport" content="width=device-width, initial-scale=1">'
        '<title>%s — FlashVM docs</title><link rel="stylesheet" href="/assets/style.css">'
        '</head><body><aside><a class="brand" href="/">FlashVM docs</a>'
        '<input id="filter" aria-label="Filter document paths" placeholder="Filter / Поиск">'
        '%s</aside><main><header><span>%s</span> <a href="/raw/%s">Raw</a></header>'
        '<article>%s</article></main><script src="/assets/viewer.js"></script></body></html>'
    ) % (escape(path), nav, escape(path), url_path(path), content)


class Handler(BaseHTTPRequestHandler):
    """Serve a fixed repository tree without writes, directory listings or arbitrary files."""

    server_version = "FlashVMDocs/1"

    def send_bytes(self, data, content_type):
        """Send an explicit-length response with restrictive browser execution policy."""
        self.send_response(200)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(data)))
        self.send_header("X-Content-Type-Options", "nosniff")
        self.send_header(
            "Content-Security-Policy",
            "default-src 'none'; style-src 'self'; script-src 'self'; img-src 'self'; base-uri 'none'; frame-ancestors 'none'",
        )
        self.end_headers()
        if self.command != "HEAD":
            self.wfile.write(data)

    def do_HEAD(self):
        """Use the same validated route and headers without sending its body."""
        self.do_GET()

    def do_GET(self):
        """Resolve assets, Markdown, source and explicitly allowed raster images."""
        try:
            request = unquote(urlsplit(self.path).path)
            if not isinstance(request, type("")):
                request = request.decode("utf-8", "strict")
            if request == "/":
                self.send_response(302)
                self.send_header("Location", "/view/" + getattr(self.server, "home", HOME))
                self.end_headers()
                return
            if request in ("/assets/style.css", "/assets/viewer.js"):
                name = request.rsplit("/", 1)[1]
                with io.open(os.path.join(os.path.dirname(__file__), name), "rb") as source:
                    self.send_bytes(
                        source.read(),
                        "text/css" if name.endswith(".css") else "application/javascript",
                    )
                return
            raw = request.startswith("/raw/")
            if not raw and not request.startswith("/view/"):
                raise ValueError("Unknown route")
            relative = request[5:] if raw else request[6:]
            filename = safe_file(self.server.root, relative)
            suffix = os.path.splitext(filename)[1].lower()
            image = IMAGE_TYPES.get(suffix)
            if (
                not image
                and suffix not in SOURCE_EXTENSIONS + (".md",)
                and os.path.basename(filename) not in ("LICENSE", "CMakeLists.txt", "Makefile")
            ):
                raise ValueError("Unsupported file type")
            if os.path.getsize(filename) > 4 * 1024 * 1024:
                raise ValueError("File exceeds the 4 MiB viewing limit")
            with io.open(filename, "rb") as source:
                data = source.read()
            if image:
                self.send_bytes(data, image)
            elif raw:
                self.send_bytes(data, "text/plain; charset=utf-8")
            else:
                text = data.decode("utf-8", "replace")
                content = (
                    render_markdown(text) if suffix == ".md" else render_source(text, filename)
                )
                self.send_bytes(
                    page(relative, content, self.server.nav).encode("utf-8"),
                    "text/html; charset=utf-8",
                )
        except (ValueError, IOError, OSError, UnicodeError):
            self.send_error(404, "Document not available")


def main(root=None, component=""):
    """Validate dependencies and start an explicitly local server until Ctrl+C."""
    root = os.path.realpath(root or ROOT)
    parser = argparse.ArgumentParser(description="Browse FlashVM Markdown and source locally")
    parser.add_argument("--port", type=int, default=8765, help="Loopback port; default 8765")
    parser.add_argument(
        "--no-browser", action="store_true", help="Do not open a browser automatically"
    )
    args = parser.parse_args()
    if not 0 <= args.port <= 65535:
        parser.error("--port must be between 0 and 65535")
    try:
        import markdown2
        import pygments
    except ImportError:
        parser.exit(
            1,
            "Install dependencies: python -m pip install -r %s\n"
            % os.path.join(PROJECT_ROOT, "doku_viewer", "requirements.txt"),
        )
    try:
        server = HTTPServer(("127.0.0.1", args.port), Handler)
    except (IOError, OSError) as error:
        parser.exit(1, "Cannot start server: %s\n" % error)
    server.root = root
    server.home = os.path.join(component, "README.md").replace(os.sep, "/")
    server.nav = navigation(root, component)
    address = "http://127.0.0.1:%d/" % server.server_address[1]
    print("FlashVM documentation: " + address)
    print("Stop with Ctrl+C. Files are served read-only from: " + root)
    if not args.no_browser:
        timer = threading.Timer(0.3, webbrowser.open, args=(address,))
        timer.daemon = True
        timer.start()
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nStopped.")
    finally:
        server.server_close()
