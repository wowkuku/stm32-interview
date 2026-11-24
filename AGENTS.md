# AGENTS.md —— 给接手的 AI / 开发者的交接说明

> **任何 AI 进入本仓库，先读本文件，再读 `README.md`。** 本文只回答三件事：项目在哪、怎么改、边界在哪。

## 0. 一句话定位

仓库 `wowkuku/stm32-interview`（**public**）本身就是线上站点：HTML / CSS / JS 放在仓库根目录，GitHub Pages 直接托管，**没有任何构建步骤**。

- 线上地址：https://wowkuku.github.io/stm32-interview/
- 默认分支：`main`。推到 `main` 即触发重新部署，**约 30 秒 ~ 1 分钟**生效。

## 1. 文件结构

```
stm32-interview/
├─ index.html                     目录页（题目列表 + 站点信息与更新日志）
├─ 01-ROM-RAM-Register.html       题号页，命名规则 NN-ASCII-NAME.html
├─ 00-demo-gpio-push-pull.html    格式示例页（非题号，定版式用）
├─ assets/style.css               全部样式（浅色主题，配色在 :root 变量）
├─ assets/app.js                  C 代码着色 / 复制按钮 / 目录滚动高亮 / 阅读进度
├─ code/                          可编译 C 源码（页内代码块是节选）
├─ README.md                      项目总说明
├─ AGENTS.md                      本文件
└─ .nojekyll                      必须存在：关闭 Jekyll，保证文件原样托管
```

## 2. 权限前提（最重要，先读这一节）

**读**这个仓库不需要任何授权（它是 public）。
**写 / 删**需要对仓库有 `write` 权限。按所处环境分三种情况：

| 环境 | 能否写入 | 前提 |
|---|---|---|
| 本机 WorkBuddy（`gh` 已登录 `wowkuku`） | **能** | 直接 `git clone` / `git push`，或 `gh api` |
| 别的 AI 产品里已连接 GitHub 并授权本仓库 | **能** | 连接器绑定的账号对该仓库有写权限 |
| 只拿到网页链接的 AI | **不能** | 网站链接是只读快照，不含任何写权限 |

**如果你只有网页链接**：不要尝试绕过，直接告诉用户「需要授权 GitHub 访问这个仓库，我才能改」。这是唯一诚实的回答。

**凭据安全**：不要把 Personal Access Token 粘贴进对话。需要独立凭据时用 fine-grained token，只勾这一个仓库、只给 `Contents: Read and write`、设置过期时间。

## 3. 加一页（标准流程）

1. 以 `01-ROM-RAM-Register.html` 为模板，复制为 `NN-ASCII-NAME.html`（NN 两位数字，文件名纯 ASCII，不含中文与空格）
2. 保持**六块结构**，顺序不许调换：
   ① 面试题干 ② 30 秒版答案 ③ 思路解析（图 / 交互件）④ 可运行源码 ⑤ 关键要点 ⑥ 追问与扣分点，页尾一句「记忆锚」
3. 在 `index.html` 的 `<div class="plist" id="plist">` 里追加一条 `<a class="pitem">`
4. 在 `index.html` 的 `#site` 区块的**更新日志表**追加一行
5. 可编译源码放进 `code/`，页内代码块给节选并注明
6. 提交后 `curl` 新页 URL 确认 200

## 4. 删一页（标准流程）

1. `git rm NN-ASCII-NAME.html`
2. 从 `index.html` 的 `#plist` 删除对应 `<a class="pitem">`
3. 若该页有专属 `code/` 文件且无其他页面引用，一并 `git rm`
4. 在更新日志表记一行「删除 NN」
5. **不要删** `assets/`、`.nojekyll`、`README.md`、`AGENTS.md`、`index.html`

## 5. 硬约束

- **零依赖**：不引入 CDN、框架、外链字体。双击 HTML 即可离线打开。
- **颜色只改** `assets/style.css` 的 `:root` 变量；内联 SVG 的 `fill` 除外。
- **代码块**：HTML 特殊字符必须转义；不需要着色的块（编译输出、终端记录）加 `data-raw`。
- **引号**：正文用中文引号；代码与属性内必须是 ASCII 直引号。
- **目录**：新增 `<h2 id="..." data-toc="...">` 才会自动进左侧目录（`app.js` 从 `data-toc` 取标题）。
- **禁止提交托管平台注入物**：`data-page-node-id`、`<!--pnid:-->`、`/page/page_comm/inject.js`。出现即说明内容是从 WorkBuddy 发布快照同步来的，必须清除（该脚本路径在 GitHub 上必然 404）。
- `.nojekyll` 缺失会导致 Jekyll 介入，可能造成文件 404。
- 提交信息用中文，格式 `type(范围): 说明`。

## 6. 改完怎么验（别只看 HTTP 200）

```bash
U=https://wowkuku.github.io/stm32-interview
curl -s -o /dev/null -w '%{http_code}\n' "$U/"          # 200
curl -s -o /dev/null -w '%{http_code}\n' "$U/新页面.html"  # 200
curl -s "$U/01-ROM-RAM-Register.html" | grep -c tk-k     # >0 说明 app.js 真的执行了
```

- **部署有约 30 秒传播窗口**：首页已更新而子页仍 404 属于正常中间态，等 `pages.status=built` 且子页 200 再判定，不要在此刻报「站点坏了」。
- 字节数与本地不一致时先查 `core.autocrlf`（CRLF→LF 归一），不是内容丢失。

## 7. 事实口径

- 芯片 **STM32F103**，容量按 F103C8T6（64KB Flash / 20KB SRAM）；寄存器地址取自 **RM0008**。
- 库以标准外设库为主，同时给裸寄存器写法（面试手写用）。F4 / H7 差异须在页内单独标注。
- **数字必须可溯源**：PC 侧结论给 gcc 实跑输出，MCU 侧代码给 Keil AC5 / AC6 编译结果。改数字前先复跑。