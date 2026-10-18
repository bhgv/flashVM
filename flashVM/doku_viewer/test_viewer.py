# -*- coding: utf-8 -*-
"""Cross-version HTTP, Markdown, source and repository-confinement regression tests."""
from __future__ import unicode_literals
import io
import os
import shutil
import tempfile
import threading
import subprocess
import sys
import re
import unittest
try:
    from urllib.request import urlopen
    from urllib.error import HTTPError
except ImportError:
    from urllib2 import urlopen, HTTPError
from doku_viewer.server import (Handler, HTTPServer, ROOT, navigation, render_markdown,
                                render_source, safe_file, url_path)


class ViewerTests(unittest.TestCase):
    """Exercise actual server routes as well as renderer security boundaries."""
    @classmethod
    def setUpClass(cls):
        """Start an isolated loopback server with an ephemeral port."""
        cls.server = HTTPServer(('127.0.0.1', 0), Handler)
        cls.server.root = ROOT
        cls.server.nav = navigation(ROOT)
        cls.thread = threading.Thread(target=cls.server.serve_forever)
        cls.thread.daemon = True
        cls.thread.start()
        cls.base = 'http://127.0.0.1:%d' % cls.server.server_address[1]

    @classmethod
    def tearDownClass(cls):
        """Stop the server and join its thread before releasing its socket."""
        cls.server.shutdown()
        cls.thread.join()
        cls.server.server_close()

    def test_root_launcher_from_another_directory(self):
        """Delegate flags/imports from the workspace launcher and open FlashVM docs."""
        directory = tempfile.mkdtemp(prefix='flashvm-launcher-')
        environment = dict(os.environ, PYTHONUNBUFFERED='1')
        process = subprocess.Popen(
            [sys.executable, os.path.join(os.path.dirname(ROOT), 'view_docs.py'),
             '--no-browser', '--port', '0'], cwd=directory, env=environment,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        try:
            first = process.stdout.readline().decode('utf-8')
            match = re.search(r'http://127\.0\.0\.1:[0-9]+/', first)
            self.assertIsNotNone(match, first)
            response = urlopen(match.group(0))
            self.assertTrue(response.geturl().endswith('/view/flashVM/README.md'))
            self.assertIn('flashVM/doc/', response.read().decode('utf-8'))
            response.close()
            license_page = urlopen(match.group(0) + 'raw/LICENSE')
            self.assertIn('MIT License', license_page.read().decode('utf-8'))
            license_page.close()
            grammar_page = urlopen(match.group(0) + 'view/compiler/as1/frontend/as1.atg')
            self.assertIn('id="L1"', grammar_page.read().decode('utf-8'))
            grammar_page.close()
        finally:
            process.terminate()
            process.communicate()
            shutil.rmtree(directory)

    def test_detached_component(self):
        """Run a renamed component without workspace files or parent access."""
        directory = tempfile.mkdtemp(prefix='detached-vm-')
        component = os.path.join(directory, 'renamed-vm')
        os.mkdir(component)
        shutil.copytree(os.path.join(ROOT, 'doku_viewer'),
                        os.path.join(component, 'doku_viewer'))
        shutil.copy(os.path.join(ROOT, 'view_docs.py'), component)
        with io.open(os.path.join(component, 'README.md'), 'w') as target:
            target.write('# Independent VM')
        with io.open(os.path.join(directory, 'secret.txt'), 'w') as target:
            target.write('outside component')
        process = subprocess.Popen(
            [sys.executable, os.path.join(component, 'view_docs.py'),
             '--no-browser', '--port', '0'], cwd=directory,
            env=dict(os.environ, PYTHONUNBUFFERED='1'),
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        try:
            first = process.stdout.readline().decode('utf-8')
            address = re.search(r'http://127\.0\.0\.1:[0-9]+/', first).group(0)
            response = urlopen(address)
            self.assertTrue(response.geturl().endswith('/view/README.md'))
            self.assertIn('Independent VM', response.read().decode('utf-8'))
            response.close()
            for path in ('raw/secret.txt', 'raw/%2e%2e/secret.txt'):
                with self.assertRaises(HTTPError) as error:
                    urlopen(address + path)
                self.assertEqual(error.exception.code, 404)
                error.exception.close()
        finally:
            process.terminate()
            process.communicate()
            shutil.rmtree(directory)

    def test_http_navigation(self):
        """Follow project, guide, example and exact source-line destinations."""
        for path in ('/', '/view/README.md', '/view/doc/STAGE_QUALITY.md',
                     '/view/examples/embedding/stage_quality/README.md',
                     '/view/include/flashvm/stage.h#L19'):
            response = urlopen(self.base + path)
            self.assertEqual(response.getcode(), 200)
            self.assertIn('text/html', response.info().get('Content-Type'))
            body = response.read().decode('utf-8')
            self.assertIn('FlashVM docs', body)
            if 'stage.h' in path:
                self.assertIn('id="L19"', body)
                self.assertIn('class="', body)
            response.close()

    def test_assets_raw_and_denials(self):
        """Serve local assets/raw text but reject hidden, escaping and executable files."""
        for path in ('/assets/style.css', '/assets/viewer.js', '/raw/VERSION.txt'):
            response = urlopen(self.base + path)
            self.assertTrue(response.read())
            self.assertIn("default-src 'none'", response.info().get('Content-Security-Policy'))
            response.close()
        for path in ('/view/../LICENSE', '/view/%2e%2e/LICENSE', '/raw/.git/config',
                     '/view/README.md/../../etc/passwd', '/unknown'):
            with self.assertRaises(HTTPError) as error:
                urlopen(self.base + path)
            self.assertEqual(error.exception.code, 404)
            error.exception.close()

    def test_markdown_and_sanitizer(self):
        """Keep tables, fences, IDs and small text while removing active HTML."""
        text = ('# Heading\n\n## Русский\n\n## Русский\n\n| A | B |\n|---|---|\n| x | y |\n\n'
                '```c\nint x;\n```\n\n<a id="target"></a><small><em>Описание</em></small>'
                '<script>alert(1)</script><img src="x.png" onerror="alert(1)">'
                '<a href="javascript:alert(1)">bad</a>')
        html = render_markdown(text)
        for expected in ('<table>', '<code>', 'id="heading"', 'id="target"', '<small>', 'Описание', 'id="русский"', 'id="русский-1"'):
            self.assertIn(expected, html)
        for forbidden in ('<script', 'onerror=', 'href="javascript:'):
            self.assertNotIn(forbidden, html)

    def test_heading_anchor_collisions(self):
        """Suffix repeated headings without colliding with literal suffixed headings."""
        html = render_markdown('# API\n\n# API-1\n\n# API\n\n# API-2\n\n# API\n')
        import re
        ids = re.findall(r'<h1 id="([^" ]+)"', html)
        self.assertEqual(ids, ['api', 'api-1', 'api-2', 'api-2-1', 'api-3'])
        self.assertEqual(len(ids), len(set(ids)))
        # Slug state belongs to one document, never a previous render/request.
        again = render_markdown('# API\n')
        self.assertIn('id="api"', again)
        self.assertNotIn('id="api-1"', again)

    def test_source_escape_and_physical_lines(self):
        """Keep physical source numbering and escape HTML inside highlighted strings."""
        html = render_source('/* one\ntwo */\nchar *x = "<script>";\n', 'x.c')
        self.assertIn('id="L3"', html)
        self.assertNotIn('id="L4"', html)
        self.assertNotIn('<script>', html)
        self.assertIn('&lt;script&gt;', html)
        blank_lines = render_source('\n\nint x;\n\n\n', 'x.c')
        self.assertIn('id="L5"', blank_lines)
        self.assertNotIn('id="L6"', blank_lines)

    def test_unicode_path_and_symlink(self):
        """Handle UTF-8 paths and reject symlinks escaping a supplied repository root."""
        folder = tempfile.mkdtemp()
        outside = tempfile.mkdtemp()
        try:
            filename = os.path.join(folder, 'пример.md')
            with io.open(filename, 'w', encoding='utf-8') as target:
                target.write('# Пример')
            self.assertEqual(safe_file(folder, 'пример.md'), filename)
            self.assertIn('%D0', url_path('пример.md'))
            original_root = self.server.root
            self.server.root = folder
            try:
                response = urlopen(self.base + '/view/' + url_path('пример.md'))
                body = response.read().decode('utf-8')
                response.close()
                self.assertIn('Пример', body)
            finally:
                self.server.root = original_root
            if hasattr(os, 'symlink'):
                os.symlink(outside, os.path.join(folder, 'escape'))
                with io.open(os.path.join(outside, 'secret.txt'), 'w') as target:
                    target.write('secret')
                with self.assertRaises(ValueError):
                    safe_file(folder, 'escape/secret.txt')
        finally:
            shutil.rmtree(folder)
            shutil.rmtree(outside)


if __name__ == '__main__':
    unittest.main()
