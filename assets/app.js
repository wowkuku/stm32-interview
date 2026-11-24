/* ============================================================
   STM32 面试题精讲 · 共用脚本（无依赖 / 离线可用）
   1) C 代码轻量着色  2) 代码复制  3) 目录自动生成 + 滚动高亮  4) 阅读进度
   ============================================================ */
(function () {
  'use strict';

  /* ---------- 1. 轻量 C 语法着色 ---------- */
  var RE = /("(?:[^"\\\n]|\\.)*"|'(?:[^'\\\n]|\\.)*')|(\/\*[\s\S]*?\*\/|\/\/[^\n]*)|(^[ \t]*#[^\n]*)|(\b0[xX][0-9a-fA-F]+[uUlL]*\b|\b\d+(?:\.\d+)?[uUlLfF]*\b)|(\b(?:if|else|for|while|do|switch|case|default|break|continue|return|void|int|char|short|long|float|double|unsigned|signed|const|static|volatile|struct|union|enum|typedef|sizeof|extern|inline|register|goto)\b)|(\b[A-Z][A-Z0-9_]{2,}\b)|(\b[a-zA-Z_]\w*_t\b)/gm;

  function esc(s) {
    return s.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
  }

  function paint(src) {
    return esc(src).replace(RE, function (m, str, com, pre, num, kw, mac, type) {
      if (str) return '<span class="tk-s">' + str + '</span>';
      if (com) return '<span class="tk-c">' + com + '</span>';
      if (pre) return '<span class="tk-p">' + pre + '</span>';
      if (num) return '<span class="tk-n">' + num + '</span>';
      if (kw) return '<span class="tk-k">' + kw + '</span>';
      if (mac) return '<span class="tk-m">' + mac + '</span>';
      if (type) return '<span class="tk-t">' + type + '</span>';
      return m;
    });
  }

  function highlightAll() {
    var blocks = document.querySelectorAll('figure.code code');
    for (var i = 0; i < blocks.length; i++) {
      var el = blocks[i];
      /* 允许用 data-raw 关掉着色（例如伪代码/输出） */
      if (el.hasAttribute('data-raw')) {
        el.innerHTML = esc(el.textContent);
        continue;
      }
      el.innerHTML = paint(el.textContent);
    }
  }

  /* ---------- 2. 复制按钮 ---------- */
  function wireCopy() {
    var btns = document.querySelectorAll('figure.code .copy');
    for (var i = 0; i < btns.length; i++) {
      (function (btn) {
        btn.addEventListener('click', function () {
          var fig = btn.closest('figure.code');
          var code = fig ? fig.querySelector('code') : null;
          if (!code) return;
          var text = code.textContent;
          var done = function () {
            var old = btn.textContent;
            btn.textContent = '已复制';
            setTimeout(function () { btn.textContent = old; }, 1200);
          };
          if (navigator.clipboard && navigator.clipboard.writeText) {
            navigator.clipboard.writeText(text).then(done, fallback);
          } else { fallback(); }

          function fallback() {
            var ta = document.createElement('textarea');
            ta.value = text;
            ta.style.position = 'fixed';
            ta.style.opacity = '0';
            document.body.appendChild(ta);
            ta.select();
            try { document.execCommand('copy'); done(); } catch (e) { /* 忽略 */ }
            document.body.removeChild(ta);
          }
        });
      })(btns[i]);
    }
  }

  /* ---------- 3. 目录 + 滚动高亮 ---------- */
  function buildToc() {
    var nav = document.getElementById('toc');
    if (!nav) return;
    var heads = document.querySelectorAll('main h2[id]');
    if (!heads.length) { nav.innerHTML = ''; return; }

    var html = '';
    var items = [];
    for (var i = 0; i < heads.length; i++) {
      var h = heads[i];
      var label = h.getAttribute('data-toc') || h.textContent.trim();
      html += '<a href="#' + h.id + '">' + label + '</a>';
      items.push({ id: h.id, el: h });
    }
    nav.innerHTML = html;

    var links = nav.querySelectorAll('a');
    var map = {};
    for (var j = 0; j < links.length; j++) map[links[j].getAttribute('href').slice(1)] = links[j];

    function activate(id) {
      for (var k = 0; k < links.length; k++) links[k].classList.remove('active');
      if (map[id]) map[id].classList.add('active');
    }

    if ('IntersectionObserver' in window) {
      var visible = {};
      var io = new IntersectionObserver(function (entries) {
        for (var n = 0; n < entries.length; n++) {
          visible[entries[n].target.id] = entries[n].isIntersecting ? entries[n].intersectionRatio : 0;
        }
        var best = null, bestVal = 0;
        for (var id in visible) { if (visible[id] > bestVal) { bestVal = visible[id]; best = id; } }
        if (best) activate(best);
      }, { rootMargin: '-72px 0px -55% 0px', threshold: [0, .2, .5, 1] });
      for (var q = 0; q < items.length; q++) io.observe(items[q].el);
    } else {
      window.addEventListener('scroll', function () {
        var cur = items[0].id;
        for (var r = 0; r < items.length; r++) {
          if (items[r].el.getBoundingClientRect().top <= 90) cur = items[r].id;
        }
        activate(cur);
      });
    }
  }

  /* ---------- 4. 阅读进度 ---------- */
  function wireProgress() {
    var bar = document.getElementById('progress');
    if (!bar) return;
    function upd() {
      var h = document.documentElement.scrollHeight - window.innerHeight;
      var p = h > 0 ? (window.scrollY / h) * 100 : 0;
      bar.style.width = Math.min(100, Math.max(0, p)) + '%';
    }
    window.addEventListener('scroll', upd, { passive: true });
    window.addEventListener('resize', upd);
    upd();
  }

  document.addEventListener('DOMContentLoaded', function () {
    highlightAll();
    wireCopy();
    buildToc();
    wireProgress();
  });
})();
/* ---------- 左码右讲联动（.codewalk） ---------- */
(function () {
  function initCodeWalk() {
    var boxes = document.querySelectorAll('.codewalk');
    for (var b = 0; b < boxes.length; b++) {
      (function (box) {
        var lines = box.querySelectorAll('.cw-line');
        var cards = box.querySelectorAll('.cw-card');
        for (var i = 0; i < lines.length; i++) {
          var no = lines[i].querySelector('.cw-no');
          if (no) no.textContent = i + 1;
        }
        function pick(step, fromLine) {
          for (var j = 0; j < lines.length; j++) {
            lines[j].classList.toggle('on', lines[j].getAttribute('data-step') === step);
          }
          for (var k = 0; k < cards.length; k++) {
            cards[k].classList.toggle('on', cards[k].getAttribute('data-step') === step);
          }
          if (fromLine) {
            var card = box.querySelector('.cw-card[data-step="' + step + '"]');
            if (card && card.scrollIntoView) card.scrollIntoView({ block: 'nearest', behavior: 'smooth' });
          }
        }
        for (var m = 0; m < lines.length; m++) {
          (function (el) {
            el.addEventListener('click', function () { pick(el.getAttribute('data-step'), true); });
          })(lines[m]);
        }
        for (var n = 0; n < cards.length; n++) {
          (function (el) {
            el.addEventListener('click', function () { pick(el.getAttribute('data-step'), false); });
          })(cards[n]);
        }
        if (lines.length) pick(lines[0].getAttribute('data-step'), false);

        var btn = box.querySelector('.cw-copy');
        if (btn) {
          btn.addEventListener('click', function () {
            var txt = [];
            for (var p = 0; p < lines.length; p++) {
              var c = lines[p].querySelector('code');
              if (c) txt.push(c.textContent);
            }
            var s = txt.join('\n');
            var done = function () {
              var old = btn.textContent;
              btn.textContent = '已复制';
              setTimeout(function () { btn.textContent = old; }, 1200);
            };
            if (navigator.clipboard && navigator.clipboard.writeText) {
              navigator.clipboard.writeText(s).then(done, done);
            } else { done(); }
          });
        }
      })(boxes[b]);
    }
  }
  if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', initCodeWalk);
  else initCodeWalk();
})();
