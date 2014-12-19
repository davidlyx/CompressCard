copy D:\ÎÄµµ\Ñ¹Ëõ¿¨\src\CompressCard\compress_card\drv\objchk_win7_amd64\amd64\drv.sys D:\ÎÄµµ\Ñ¹Ëõ¿¨\src\CompressCard\install\x64\drv.sys
C:\WinDDK\7600.16385.1\bin\selfsign\Inf2Cat.exe /driver:D:\ÎÄµµ\Ñ¹Ëõ¿¨\src\CompressCard\install /os:7_X64
C:\WinDDK\7600.16385.1\bin\amd64\SignTool.exe sign /v /s MicroFat /n MicroFat /t http://timestamp.verisign.com/scripts/timestamp.dll D:\ÎÄµµ\Ñ¹Ëõ¿¨\src\CompressCard\install\drv.cat
