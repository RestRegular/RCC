# serr-builtin 套件

验证新增内建 `serr`（standard error output）：

- 与 `sout` 同构（多实参、end=、全类型 int/float/bool/char/str/ptr/null、默认换行）
- 唯一区别是写到 stderr（fd=2）而非 stdout（fd=1）

> 驱动 `tests/_shared/driver.ps1` 用 `2>&1` 合并两路输出，因此本套件用
> `stderrContains` 断言 serr 文本出现；stdout/stderr 的物理分离另由
> 手工 `cmd /c "... >out 2>err"` 验证（见记忆 e260f679）。
