# g1b-argv 套件

验证 G1b（2026-11-20 裁决，A+B 混合）：

- `prog.args(): list[str]`：进程级 argv（不含程序名）；无参时为空 list
- `prog.entry(callback, args...)`：编译期入口文件守卫——入口文件调用时转发实参给回调；
  被 import 的模块里调用时为 no-op

> 驱动不传运行时实参，故 argv 用例验证「无参 → 空 list」；带参场景由
> `tests/g1b-probe/` 手工验证（含空格的参数逐字节正确）。
