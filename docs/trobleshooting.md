1. clangd语言服务器找不到c++ std头文件
```
 sudo apt install g++-12
```
2. 编译器依赖找不到
使用xmake构建系统，可以在读取配置文件后自动安装依赖

3. 规范里要求实现命令行互动
实现了命令行的输入监听, 命令的parser以及protocol interpreter, 现在服务器和客户端知道了取得命令后需要执行什么

