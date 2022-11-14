# FTP说明

#### 1、使用方法：

1. 在client_linux目录下执行：`make`命令进行编译、`make clean`命令删除可运行文件、`sudo ./client localhost`命令运行客户端程序。
2. 在server_linux目录下执行：`make`命令进行编译、`make clean`命令删除可运行文件、`sudo ./server`命令运行客户端程序。
3. 登录名和密码在`FTPInC`目录下的隐藏文件`.passwd`中。每一条记录包含用户名、密码、权限。
4. 目前实现指令delete、ls、cd、mkdir、pwd
5. cd（切换当前目录）操作需输入全路径，如切换到user_dir下的test文件夹，输入路径为"/user_dir/test"，或使用"~"基于当前位置进行切换，如在user_dir文件夹，输入路径为"~/test"。
6. mkdir目前仅可用于在当前位置下新建文件夹。
7. delete目前仅可用于在当前位置下删除文件，目的是防止用户删除user_dir，或是在文件夹内删除该文件夹之类的操作。
7. 目前cd、mkdir、delete均改为了"指令 文件/目录名"的格式，cd若传入空值则回到user_dir目录。

#### 2、避坑

1. send()函数的**buf**长度需要等于recv()函数的**buf**长度，否则下一次会读取到乱码。
2. 定义新数组千万记得用bzero(char_name, sizeof(char_name))先置0，否则可能会有奇怪的数据。
3. get和put的文件路径设置为不能包含斜杠，由于c打开文件路径如果文件夹不存在就会打开失败，导致file指针为空，fwrite会发生段错误（我用gdb调试发现的，printf大法果然还是太弱了）
3. get和put的核心功能在common文件夹中，所以相对路径要先返回到FTPInC再定位(好像不用，但是我实现了保险一点)

#### 3、待后续实现功能

1. 目前已有一个基础的list指令可以使用，显示的是user_dir文件夹下的文件，希望change dir命令只能在user_dir文件夹中切换，不能跳出该文件夹。