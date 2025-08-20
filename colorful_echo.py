import sys

from newrcc.CConsole import colorfulText
from newrcc.CColor import TextColor


if __name__ == '__main__':
    print(colorfulText(' '.join(sys.argv[1:]), TextColor.BLUE))
