#!/usr/bin/env python3
"""
编码转换工具：将指定目录下所有 .h 和 .cpp 文件从 GB18030 编码转换为 UTF-8 编码
"""

import os
import sys
from pathlib import Path


def convert_file_encoding(file_path, src_encoding='GB18030', dst_encoding='UTF-8'):
    """
    转换单个文件的编码

    Args:
        file_path: 文件路径
        src_encoding: 源编码
        dst_encoding: 目标编码

    Returns:
        bool: 转换是否成功
    """
    try:
        # 读取原文件
        with open(file_path, 'r', encoding=src_encoding) as f:
            content = f.read()

        # 用新编码写回
        with open(file_path, 'w', encoding=dst_encoding, newline='') as f:
            f.write(content)

        print(f"✓ 已转换: {file_path}")
        return True

    except UnicodeDecodeError as e:
        print(f"✗ 编码错误 {file_path}: {e}")
        return False
    except Exception as e:
        print(f"✗ 错误 {file_path}: {e}")
        return False


def convert_directory(root_dir, extensions=None, dry_run=False):
    """
    递归扫描目录并转换文件编码

    Args:
        root_dir: 根目录路径
        extensions: 要处理的文件扩展名列表，默认为 ['.h', '.cpp']
        dry_run: 如果为 True，只列出文件但不执行转换
    """
    if extensions is None:
        extensions = ['.h', '.cpp']

    root_path = Path(root_dir)

    if not root_path.exists():
        print(f"错误：目录不存在 - {root_dir}")
        return

    if not root_path.is_dir():
        print(f"错误：不是有效的目录 - {root_dir}")
        return

    # 收集所有匹配的文件
    files_to_convert = []
    for ext in extensions:
        # 使用 rglob 递归搜索，不区分大小写
        for file_path in root_path.rglob(f'*{ext}'):
            files_to_convert.append(file_path)
        for file_path in root_path.rglob(f'*{ext.upper()}'):
            if file_path not in files_to_convert:
                files_to_convert.append(file_path)

    if not files_to_convert:
        print(f"在 {root_dir} 中没有找到匹配的文件")
        return

    print(f"找到 {len(files_to_convert)} 个文件{' (预览模式)' if dry_run else ''}:")
    print("-" * 60)

    success_count = 0
    fail_count = 0

    for file_path in sorted(files_to_convert):
        if dry_run:
            print(f"  {file_path}")
        else:
            if convert_file_encoding(str(file_path)):
                success_count += 1
            else:
                fail_count += 1

    # 打印汇总信息
    print("-" * 60)
    if dry_run:
        print(f"预览完成。如需执行转换，请去掉 dry_run 参数")
    else:
        print(f"转换完成! 成功: {success_count}, 失败: {fail_count}")


def main():
    """主函数"""
    import argparse

    parser = argparse.ArgumentParser(
        description='将指定目录下的 .h 和 .cpp 文件从 GB18030 转换为 UTF-8 编码'
    )
    parser.add_argument(
        'directory',
        nargs='?',
        default='.',
        help='要处理的根目录路径 (默认: 当前目录)'
    )
    parser.add_argument(
        '--extensions',
        nargs='+',
        default=['.h', '.cpp'],
        help='要处理的文件扩展名 (默认: .h .cpp)'
    )
    parser.add_argument(
        '--dry-run',
        action='store_true',
        help='只列出要处理的文件，不执行转换'
    )
    parser.add_argument(
        '--src-encoding',
        default='GB18030',
        help='源编码 (默认: GB18030)'
    )
    parser.add_argument(
        '--dst-encoding',
        default='UTF-8',
        help='目标编码 (默认: UTF-8)'
    )
    parser.add_argument(
        '--backup',
        action='store_true',
        help='转换前备份原文件'
    )

    args = parser.parse_args()

    # 如果需要备份，先创建备份
    if args.backup and not args.dry_run:
        import shutil
        root_path = Path(args.directory)
        backup_dir = root_path / f"backup_{__import__('datetime').datetime.now().strftime('%Y%m%d_%H%M%S')}"
        backup_dir.mkdir(exist_ok=True)

        files_backed_up = 0
        for ext in args.extensions:
            for file_path in root_path.rglob(f'*{ext}'):
                rel_path = file_path.relative_to(root_path)
                backup_path = backup_dir / rel_path
                backup_path.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(file_path, backup_path)
                files_backed_up += 1

        print(f"已备份 {files_backed_up} 个文件到 {backup_dir}")
        print("-" * 60)

    # 执行转换
    convert_directory(
        root_dir=args.directory,
        extensions=args.extensions,
        dry_run=args.dry_run
    )


if __name__ == '__main__':
    # 如果直接运行脚本，使用当前目录
    if len(sys.argv) == 1:
        # 没有命令行参数，直接转换当前目录
        convert_directory('.')
    else:
        main()