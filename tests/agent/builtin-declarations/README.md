# builtin-declarations 套件

验证「内建声明严格化」（2026-11-20 裁决）：

- 内建/原语可直接调用（sout/serr/clone/size/get 等，无需用户声明）
- 未声明函数编译期报错（不再拖到链接期）
- core_*/rt_* 运行时原语合法
- 用户不得给保留名写真实函数体（encapsulated 桩合法）
- 用户自定义同名非内建函数（如 release）不受误伤
