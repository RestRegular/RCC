# module-visibility 套件

验证 import/export 符号可见性改造（2026-11-20）：

- 新 import 语法：`import "mod"` 单库、`import m = "mod"` 别名
- export 白名单：有 export 时，未列入的符号对外不可见
- hidden 标签：符号对外不可见（模块内部仍可用）
- hidden + export 同一符号 → 编译报错
- 别名隔离：只能通过 alias.foo 访问，裸名不可访问
