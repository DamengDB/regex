# REGEX

该库为正则表达式库，从glibc库分离得到，提供了一些用于解析处理正则表达式的函数。

## 参考文档

https://sourceware.org/glibc.manual/2.30/

## 使用方法

可以单独编译动态库使用，以x86 centos7为例：
gcc -m64 -shared  -o libregex.so regcomp.o regex.o regex_internal.o regexec.o -Wl,-Bdynamic -L../debug -Wl,-Bdynamic -lc -lm -lrt -lpthread -ldl -Wl,-rpath,./,-rpath,../debug,--unresolved-symbols=ignore-in-shared-libs
也可以将文件加入项目使用
需要引用头文件regex.h
#include "regex.h"

## 函数定义

glibc库提供了以下四个函数
regcomp():用于编译正则表达式
ini regcomp (regex_t * preg, const char * pattern, int cflags);
preg：用于保存编译后的结果
regex：需要编译的正则表达式
cflags：处理正则表达式的标记

regexec()：利用regcomp的编译结果，进行正则表达式匹配
int regexec (const regex_t * preg, const char * string, size_t nmatch, regmatch_t pmatch[], int eflags);
preg：正则表达式编译的结果
string：需要匹配的字符串
nmatch：子表达式数
pmatch[]：匹配到的字符串位置
eflags：处理正则表达式的标记

regerror()：获取错误消息
size_t regerror (int errcode, const regex_t * preg, char * errbuf,  size_t errbuf_size);
errcode：错误码
preg：正则表达式编译的结果
errbuf：错误信息
errbuf_size：errbuf的缓冲区大小

regfree()：释放regex_t
void regfree (regex_t *preg);
preg：正则表达式编译的结果

dm在此基础上另外提供了两个函数
reg_set_locale()：根据数据库编码设置环境
void reg_set_locale(int charset, int flag);
charset：DM的编码集
flag：设置/清理

reg_mem_init()：设置内存管理函数，若设置内部管理内存奖励使用设置的函数
void reg_mem_init(mem_malloc_t  mem_malloc_fun, mem_realloc_t mem_realloc_fun, mem_free_t mem_free_fun, mem_calloc_t mem_calloc_fun);
mem_malloc_fun：申请内存函数
mem_realloc_fun：重新申请内存函数
mem_free_fun：释放内存函数
mem_calloc_fun：申请并初始化内存函数

## License

LGPL v2.1