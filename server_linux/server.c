#include "server.h"
#include <fcntl.h>
#include <sys/stat.h>

#define USER_DIR "/user_dir"
char manage_level = '0';
char current_dir[MAX_SIZE] = USER_DIR;

int main()
{
	int listen_fd, sock_fd, pid;

	listen_fd = init_server(LISTEN_PORT);

	while (1)
	{
		//接收连接
		sock_fd = accept_client(listen_fd);
		//创建子进程
		if ((pid = fork()) < 0)
		{
			perror("fork error");
			exit(1);
		}
		else if (pid == 0) // child process
		{
			close(listen_fd);
			work_process(sock_fd);
			close(sock_fd);
			exit(0);
		}
		close(sock_fd);
	}
	close(listen_fd);
	return 0;
}

void work_process(int sock_fd)
{
	int work_fd;
	char cmd[10], arg[MAX_SIZE];

	send_num(sock_fd, CONN_SUCCESS);

	int ret;
	char loginOrRegist[MAX_SIZE];

	while (1)
	{
		bzero(loginOrRegist, sizeof(loginOrRegist));
		// login & register?
		if ((ret = recv_data(sock_fd, loginOrRegist, sizeof(loginOrRegist))) < 0)
		{
			perror("recv user error:");
			exit(1);
		}
		if (strcmp(loginOrRegist, "REGISTER") == 0)
		{
			ret = server_register(sock_fd);
			if (ret == REGIST_NAME_REPEAT)
			{
				send_num(sock_fd, REGIST_NAME_REPEAT);
				continue;
			}
			if (ret == 0)
			{
				send_num(sock_fd, REGIST_REFUSED);
			}
			else
			{
				send_num(sock_fd, REGIST_SUCCESS);
			}
			continue;
		}
		if (strcmp(loginOrRegist, "LOGIN") == 0)
		{
			ret = server_login(sock_fd);
			//认证失败
			if (ret != 1)
			{
				send_num(sock_fd, LOGIN_FAILED);
				continue;
			}
			else
			{
				send_num(sock_fd, LOGIN_SUCCESS);
				break;
			}
		}
	}

	//处理请求
	while (1)
	{
		//接收客户端的请求并解析
		int ret_code = server_get_request(sock_fd, cmd, arg); 
		if ((ret_code < 0) || (ret_code == QUIT_SUCESS))
			break;
		if (ret_code == CMD_SUCCESS)
		{
			//创建与客户端的数据连接,之前的是监听连接
			work_fd = server_work_conn(sock_fd);
			if (work_fd < 0)
			{
				close(sock_fd);
				printf("work_fd创建失败");
				exit(1);
			}
			//创建了数据连接之后,执行对应的命令即可
			if (strcmp(cmd, "LS") == 0)
			{
				server_cmd_ls(work_fd, sock_fd);
			}
			else if (strcmp(cmd, "PWD") == 0)
			{
				server_cmd_pwd(work_fd, sock_fd);
			}
			else if (strcmp(cmd, "MKDIR") == 0)
			{
				server_cmd_mkdir(work_fd, sock_fd);
			}
			else if (strcmp(cmd, "CD") == 0)
			{
				server_cmd_cd(work_fd, sock_fd);
			}
			else if (strcmp(cmd, "DELETE") == 0)
			{
				server_cmd_delete(work_fd, sock_fd);
			}
			else if (strcmp(cmd, "GET") == 0)
			{
				server_cmd_get(work_fd, sock_fd, arg);
			}
			else if (strcmp(cmd, "PUT") == 0)
			{
				server_cmd_put(work_fd, sock_fd, arg);
			}

			close(work_fd);
		}
	}
}

int server_check(char *username, char *password)
{
	FILE *file = fopen("../.passwd", "r");
	int i, j;
	int result;
	if (!file)
	{
		printf("文件打开失败！");
		exit(1);
	}
	char temp[50];
	char name[20];
	while (1)
	{
		if (feof(file))
		{
			result = 0;
			break;
		}
		i = 0;
		j = 0;
		fgets(temp, 49, file);
		while (temp[i] != ',')
		{
			name[i] = temp[i];
			i++;
		}
		name[i] = '\0';
		if (strcmp(name, username) != 0)
		{
			continue;
		}
		else
		{
			char userpassword[20];
			for (i += 1; temp[i] != ','; j++, i++)
			{
				userpassword[j] = temp[i];
			}
			userpassword[j] = '\0';
			if (strcmp(userpassword, password) != 0)
			{
				result = 0;
				break;
			}
			else
			{
				i = i + 1;
				if(temp[i]=='H') manage_level='3';
				if(temp[i]=='M') manage_level='2';
				if(temp[i]=='L') manage_level='1';
				result = 1;
				break;
			}
		}
	}
	fclose(file);
	return result;
}

void file_write(int sock_fd, char *accountName, char *au)
{
	FILE *file = fopen("../.passwd", "a");
	char buf[MAX_SIZE];
	if (!file)
	{
		printf("File open failed\n");
		exit(-1);
	}
	char accountPassword[20];
	bzero(buf, sizeof(buf));
	recv_data(sock_fd, buf, sizeof(buf));
	int i = 5;
	int j = 0;
	while (buf[i] && buf[i] != '\0')
	{
		accountPassword[j++] = buf[i++];
	}
	accountPassword[j]='\0';
	char line[50];
	line[0] = '\0';
	strcat(line, accountName);
	strcat(line, ",");
	strcat(line, accountPassword);
	strcat(line, ",");
	strcat(line, au);
	strcat(line, "\n");
	fputs(line, file);
	fclose(file);
	return;
}

int manager_check(int sock_fd, char *accountName, char *au)
{
	char getAnswer[MAX_SIZE];
	printf("A user named %s just sent a register application!\n", accountName);
	while (1)
	{
		printf("Agree please press 'y' or 'Y'，or press 'N' or 'n' to refuse:");
		scanf("%s", getAnswer);
		if (getAnswer[0] == 'y' || getAnswer[0] == 'Y')
		{
			printf("Please set permissions for this user：1（can only upload）、2（upload & download）、3（upload & download & change directory）\n");
			while (1)
			{
				printf("Please enter number（1、2、3）：");
				scanf("%s", getAnswer);
				if (getAnswer[0] == '1' || getAnswer[0] == '2' || getAnswer[0] == '3')
				{
					au[0] = '\0';
					switch (getAnswer[0])
					{
					case '3':
						strcat(au, "HIGH");
						break;
					case '2':
						strcat(au, "MID");
						break;
					case '1':
						strcat(au, "LOW");
						break;
					}
					break;
				}
				else
				{
					printf("Illegal input!\n");
				}
			}
			send_num(sock_fd, REGIST_APPLICATION_OK);
			return 1;
		}
		if (getAnswer[0] == 'n' || getAnswer[0] == 'N')
		{
			return 0;
		}
		else
		{
			printf("Illegal input!\n");
		}
	}
}

int server_register(int sock_fd)
{
	int i, ret, result, flag, j;
	char temp[MAX_SIZE];
	char name[MAX_SIZE];
	char accountName[MAX_SIZE];
	FILE *file = fopen("../.passwd", "r");
	if (!file)
	{
		printf("文件打开失败！");
		exit(1);
	}
	char buf[MAX_SIZE];
	bzero(buf, sizeof(buf));
	recv(sock_fd, buf, sizeof(buf), 0);
	i = 5;
	j = 0;
	while (buf[i])
	{
		accountName[j++] = buf[i++];
	}
	accountName[j] = '\0';
	while (1)
	{
		if (feof(file))
		{
			result = 1;
			break;
		}
		i = 0;
		fgets(temp, 59, file);
		while (temp[i] != ',')
		{
			name[i] = temp[i];
			i++;
		}
		// WORK_PORT = '\0';
		if (strcmp(name, accountName) != 0)
		{
			continue;
		}
		else
		{
			result = 0;
			break;
		}
	}
	fclose(file);
	if (result == 0)
	{
		return REGIST_NAME_REPEAT;
	}
	if (result == 1)
	{
		send_num(sock_fd, REGIST_NAME_OK);
		char au[MAX_SIZE];
		ret = manager_check(sock_fd, accountName, au);
		if (ret == 0)
			return 0;
		else
		{
			file_write(sock_fd, accountName, au);
			return 1;
		}
	}
}

int server_login(int sock_fd)
{
	int ret;
	char buf[MAX_SIZE];
	char user[MAX_SIZE];
	char passwd[MAX_SIZE];
	bzero(buf, sizeof(buf)); //数组清零
	bzero(user, sizeof(user));
	bzero(passwd, sizeof(passwd));

	//获取客户端传来的用户名
	if ((ret = recv_data(sock_fd, buf, sizeof(buf))) < 0)
	{
		perror("recv user error:");
		exit(1);
	}
	int n = 0, i = 5;
	while (buf[i])
	{
		user[n++] = buf[i++];
	}
	//通知输入密码
	send_num(sock_fd, 331);

	//获取客户端传来的密码
	bzero(buf, sizeof(buf));
	if ((ret = (recv_data(sock_fd, buf, sizeof(buf)))) < 0)
	{
		perror("recv passwd error:");
		exit(1);
	}

	i = 5;
	n = 0;
	while (buf[i])
	{
		passwd[n++] = buf[i++];
	}

	return (server_check(user, passwd));
}

int server_get_request(int sock_fd, char *cmd, char *arg)
{
	int ret_code = CMD_SUCCESS;
	char buf[MAX_SIZE];

	bzero(buf, sizeof(buf));

	//接收客户端命令
	if ((recv_data(sock_fd, buf, sizeof(buf))) == -1)
	{
		perror("recv error");
		exit(1);
	}

	printf("接收到指令:%s\n", buf);
	get_cmd_first_arg(buf, cmd, arg);

	if (strcmp(cmd, "QUIT") == 0)
	{
		ret_code = QUIT_SUCESS;
	}
	else if((strcmp(cmd, "MKDIR") == 0)||(strcmp(cmd, "DELETE") == 0))
	{
		if(manage_level != '3') 
		{
				send_num(sock_fd, OUT_OF_AUTHORITY);
				return OUT_OF_AUTHORITY;
		}
	}
	else if((strcmp(cmd, "GET") == 0) || (strcmp(cmd, "PUT") == 0))
	{
		if(manage_level == '1') 
		{
				send_num(sock_fd, OUT_OF_AUTHORITY);
				return OUT_OF_AUTHORITY;
		}
	}
	else if ((strcmp(cmd, "USER") == 0) || (strcmp(cmd, "PASS") == 0) || (strcmp(cmd, "LS") == 0) || (strcmp(cmd, "GET") == 0) || (strcmp(cmd, "PUT") == 0) || (strcmp(cmd, "PWD") == 0) || (strcmp(cmd, "MKDIR") == 0) || (strcmp(cmd, "CD") == 0) || (strcmp(cmd, "DELETE") == 0))
	{
		ret_code = CMD_SUCCESS;
	}
	else
	{
		ret_code = CMD_FAIL;
	}
	send_num(sock_fd, ret_code);
	return ret_code;
}

int server_work_conn(int sock_fd)
{
	char buf[MAX_SIZE];
	int wait, work_fd;

	if (recv(sock_fd, &wait, sizeof(wait), 0) < 0)
	{
		perror("error while wait");
		exit(1);
	}

	struct sockaddr_in client_addr;
	socklen_t len = sizeof(client_addr);
	getpeername(sock_fd, (struct sockaddr *)&client_addr, &len);
	inet_ntop(AF_INET, &client_addr.sin_addr, buf, sizeof(buf));

	//创建到客户端的数据连接
	if ((work_fd = connect_server(DATA_PORT, buf)) < 0)
	{
		exit(1);
	}
	return work_fd;
}

int server_cmd_ls(int work_fd, int sock_fd)
{
	//读取当前目录
	send_num(sock_fd, SERVER_READY);
	char data[MAX_SIZE];
	char dir[MAX_SIZE];
	//数组初始置0，否则可能会有奇怪的数据！
	bzero(dir, sizeof(dir));

	struct dirent *file;
	strcat(dir, "..");
	strcat(dir, current_dir);
	strcat(dir, "/");

	DIR *direc = opendir(dir);
	if (direc == NULL)
	{
		perror("open dir error");
		exit(1);
	}
	int n = 0;
	bzero(data, sizeof(data));
	char type[100];
	while ((file = readdir(direc)) != NULL)
	{
		if (file->d_type == DT_DIR)
		{
			type[0] = '\0';
			strcat(type, "\033[1;34;49m");
			strcat(type, file->d_name);
			strcat(type, "\033[0m");
			sprintf(data + n, "%s\n", type);
			n += strlen(type) + 1;
		}
		else
		{
			sprintf(data + n, "%s\n", file->d_name);
			n += strlen(file->d_name) + 1;
		}
	}
	data[n] = '\0';
	closedir(direc);
	if (send(work_fd, data, strlen(data), 0) < 0)
	{
		perror("send error");
	}
	send_num(sock_fd, RET_SUCCESS);
	return 0;
}

int server_cmd_pwd(int work_fd, int sock_fd)
{
	//打印当前所在目录
	send_num(sock_fd, SERVER_READY);
	char data[MAX_SIZE];
	bzero(data, sizeof(data));
	strcpy(data, current_dir);
	if (send(work_fd, data, strlen(data), 0) < 0)
	{
		perror("send error");
	}
	send_num(sock_fd, RET_SUCCESS);
	return 0;
}

int server_cmd_mkdir(int work_fd, int sock_fd)
{
	//新建文件夹
	send_num(sock_fd, SERVER_READY);
	char new_dir[MAX_SIZE];
	char get_dir[MAX_SIZE];
	bzero(new_dir, sizeof(new_dir));
	bzero(get_dir, sizeof(get_dir));
	strcat(new_dir, "..");
	strcat(new_dir, current_dir);
	strcat(new_dir, "/");

	if (recv(work_fd, get_dir, MAX_SIZE, 0) > 0)
	{
		strcat(new_dir, get_dir);
		int isCreate = mkdir(new_dir, 0775);
		if (!isCreate)
		{
			printf("create path:%s\n", new_dir);
			send_num(sock_fd, SERVER_READY);
		}
		else
		{
			printf("create path:%s failed! error code : %d \n", new_dir, isCreate);
			send_num(sock_fd, 0);
		}
	}
	else
		send_num(sock_fd, 0);
	return 0;
}

int server_cmd_cd(int work_fd, int sock_fd)
{
	//变更所在目录
	send_num(sock_fd, SERVER_READY);
	char new_path[MAX_SIZE];
	char get_path[MAX_SIZE];
	bzero(new_path, sizeof(new_path));
	bzero(get_path, sizeof(get_path));

	if (recv(work_fd, get_path, MAX_SIZE, 0) > 0)
	{
		if (get_path[0] == '~' || (strlen(get_path) == 1 && get_path[0] == '/'))
		{
			if (strlen(get_path) != 1)
			{
				send_num(sock_fd, 0);
			}
			else
			{
				current_dir[0] = '\0';
				strcat(current_dir, "/user_dir");
				send_num(sock_fd, SERVER_READY);
			}
		}
		else if (get_path[0] == '.' && get_path[1] == '.')
		{
			if (strlen(get_path) != 2)
			{
				send_num(sock_fd, 0);
			}
			else
			{
				for (int l = strlen(current_dir) - 1; l >= 0; l--)
				{
					if (current_dir[l] == '/')
					{
						if (l == 0)
						{
							send_num(sock_fd, PATH_OUT);
							return 0;
						}
						else
						{
							current_dir[l] = '\0';
							send_num(sock_fd, SERVER_READY);
						}
						break;
					}
				}
			}
		}
		else if (get_path[0] == '/')
		{
			strcat(new_path, "..");
			strcat(new_path, get_path);
			if (!access(new_path, 0))
			{
				current_dir[0] = '\0';
				strcat(current_dir, get_path);
				printf("change path to:%s\n", current_dir);
				send_num(sock_fd, SERVER_READY);
			}
			else
				send_num(sock_fd, 0);
		}
		else
		{
			strcat(new_path, "..");
			strcat(new_path, current_dir);
			strcat(new_path, "/");
			strcat(new_path, get_path);
			if (!access(new_path, 0))
			{
				strcat(current_dir, "/");
				strcat(current_dir, get_path);
				printf("change path to:%s\n", current_dir);
				send_num(sock_fd, SERVER_READY);
			}
			else
				send_num(sock_fd, 0);
		}
	}
	else
		send_num(sock_fd, -1);
	return 0;
}

int server_cmd_delete(int work_fd, int sock_fd)
{
	send_num(sock_fd, SERVER_READY);
	char delete_path[MAX_SIZE];
	char file_name[MAX_SIZE];
	bzero(delete_path, sizeof(delete_path));
	bzero(file_name, sizeof(file_name));
	if (recv(work_fd, file_name, MAX_SIZE, 0) > 0)
	{
		if (!strcmp(file_name, "..") || !strcmp(file_name, "."))
		{
			send_num(sock_fd, OUT_OF_AUTHORITY);
			return 0;
		}
		struct dirent *file;
		strcat(delete_path, "..");
		strcat(delete_path, current_dir);
		strcat(delete_path, "/");
		DIR *direc = opendir(delete_path);
		if (direc == NULL)
		{
			perror("open dir error");
			exit(1);
		}
		while ((file = readdir(direc)) != NULL)
		{
			if (!strcmp(file->d_name, file_name))
			{
				if (file->d_type == DT_DIR)
				{
					send_num(sock_fd, IS_DT_DIR);
					return 0;
				}
				else
				{
					strcat(delete_path, file_name);
					remove(delete_path);
					printf("delete %s\n", delete_path);
					send_num(sock_fd, SERVER_READY);
					return 0;
				}
			}
		}
		send_num(sock_fd, 0);
	}
	else
		send_num(sock_fd, -1);
	return 0;
}

void server_cmd_get(int work_fd, int sock_fd, char *file_name)
{
	send_num(sock_fd, SERVER_READY);

	static char filepath[MAX_SIZE] = {'\0'};
	bzero(filepath, MAX_SIZE);
	strcat(filepath, "..");
	strcat(filepath, current_dir);
	strcat(filepath, "/");
	strcat(filepath, file_name);

	send_file(work_fd, sock_fd, filepath, file_name);
	send_num(sock_fd, RET_SUCCESS);
}

void server_cmd_put(int work_fd, int sock_fd, char *file_name)
{
	send_num(sock_fd, SERVER_READY);

	static char filepath[MAX_SIZE] = {'\0'};
	bzero(filepath, MAX_SIZE);
	strcat(filepath, "..");
	strcat(filepath, current_dir);
	strcat(filepath, "/");
	strcat(filepath, file_name);

	get_file(work_fd, sock_fd, filepath);

	send_num(sock_fd, RET_SUCCESS);
}