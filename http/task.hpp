#ifndef _TASK_HPP_
#define _TASK_HPP_

#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

#ifndef BUFF_SIZE
#define BUFF_SIZE 1024 * 1024  // 1MB
#endif                         /* BUFF_SIZE */

const int BUFFSIZE = 4096;
const string path = "./Resource";

class Task {
 private:
  int client_fd;
  char order[BUFFSIZE];
  bool is_browser;

 public:
  Task(){};
  Task(int client_fd, char* str)
      : client_fd(client_fd), is_browser(false) {
    strcpy(order, str);
  };
  ~Task(){};
  void response(string message, int status);
  void response_file(int size, int status, string content_type);
  void response_post(string filename, string command);
  void response_get(string filename);
  void response_head(string filename);
  void response_delete(string filename);
  int rm(std::string file_name);
  int rm_dir(std::string dir_full_path);
  void run();

 private:
  string get_content_type(string suffix);
};

void Task::response(string message, int status) {
  stringstream header_message;
  if (status == 404) {
    if (message == "HEAD404") {
      cout << message << endl;
      header_message << "HTTP/1.1 " << to_string(status) << " Not Found\r\n"
                     << "Connnection: Close\r\n\r\n";
    } else {
      header_message << "HTTP/1.1 " << to_string(status) << " Not Found\r\n"
                     << "Connnection: Close\r\n"
                     << "Content-type: text/html\r\n"
                     << "Content-length:" << to_string(message.size())
                     << "\r\n\r\n";
    }
    string out_message;
    if (message != "HEAD404") {
      out_message = header_message.str() + message;
    } else {
      out_message = header_message.str();
    }

    const char* buffer = out_message.c_str();
    write(client_fd, buffer, out_message.size());
  } else {
    header_message << "HTTP/1.1 " << to_string(status) << " OK\r\n"
                   << "Content-type: text/html\r\n"
                   << "Content-length:" << to_string(message.size())
                   << "\r\n\r\n";
    string out_message;
    out_message = header_message.str() + message;
    const char* buffer = out_message.c_str();
    write(client_fd, buffer, out_message.size());
  }
}

void Task::response_file(int size, int status, string content_type) {
  stringstream out_message;
  out_message << "HTTP/1.1 " << to_string(status) << " OK\r\n"
              << "Connection: Close\r\n"
              << "Content-length:" << to_string(size) << "\r\n"
              << "Content-Type: " << content_type << "\r\n\r\n";
  const char* buffer = out_message.str().c_str();
  cout << buffer << endl;
  write(client_fd, buffer, out_message.str().size());
}

void Task::run() {
  char buffer[BUFFSIZE];
  strcpy(buffer, order);
  int size = 0;
  // while (client_state) {
  // size = read(client_fd, buffer, BUFFSIZE);
  // if (size > 0) {
  int i = 0;
  string method;
  string filename;
  if ((strstr(buffer, "HTTP")) != NULL) is_browser = true;
  while (buffer[i] != ' ' && buffer[i] != '\0') {
    method += buffer[i++];
  }
  i++;

  while (buffer[i] != ' ' && buffer[i] != '\0' && buffer[i] != '?') {
    filename += buffer[i++];
  }
  cout << method << endl;
  cout << filename << endl;
  // cout << buffer << endl;
  if (method == "GET") {
    response_get(filename);
  } else if (method == "HEAD") {
    response_head(filename);
  } else if (method == "POST") {
    string content;
    int pos = content.find("username=");
    while (pos == -1) {
      content.clear();
      while (buffer[i] != ' ' && buffer[i] != '\0') {
        content += buffer[i++];
      }
      i++;
      pos = content.find("username=");
    }
    string command;
    if (pos != -1)
      command = content.substr(pos, content.length() - pos);
    else
      command = "";
    cout << command << endl;
    if (filename[0] == '/' && filename.length() == 1)
      response_post("/index.html", command);
    else
      response_post(filename, command);
  } else if (method == "DELETE") {
    response_delete(filename);
  } else {
    stringstream message;
    message << "<html><title>Myhttpd Error</title>"
            << "<body>\r\n"
            << " 501\r\n"
            << " <p>" << method << "Httpd does not implement this method"
            << "<hr><h3>The Tiny Web Server<h3></body>";
    response(message.str(), 501);
  }
}
// else {
//   client_state = false;
//   continue;
// }
// }
// sleep(2);
// close(client_fd);

void Task::response_get(string filename) {
  bool is_dynamic = false;
  string command;
  string file = path;
  file += filename;
  // cout << filename << endl;
  if (filename[0] == '/' && filename.length() == 1) {
    file += "index.html";
  }
  cout << file << endl;

  struct stat filestat;
  int ret = stat(file.c_str(), &filestat);

  if (ret < 0 || S_ISDIR(filestat.st_mode)) {
    string message;
    message += "<html><title>Myhttpd Error</title>";
    message += "<body>\r\n";
    message += " 404\r\n";
    message += " <p>GET: Can't find the file";
    message += "<hr><h3>My Web Server<h3></body>";
    response(message, 404);
    return;
  }
  int filefd = open(file.c_str(), O_RDONLY);
  int pos = file.rfind('.', file.length() - 1);
  string suffix = file.substr(pos + 1, file.length() - pos);
  string content_type = get_content_type(suffix);
  cout << content_type << endl;
  if (is_browser) response_file(filestat.st_size, 200, content_type);
  cout << "filesize: " << filestat.st_size << endl;
  std::ifstream in = std::ifstream(file, ios::binary);
  struct timeval tv;
  tv.tv_sec = 1024;
  tv.tv_usec = 500;
  fd_set wset;
  ssize_t nb_read = 0;
  ssize_t nb_write = 0;
  constexpr size_t buffer_size = 1024 * 1024;
  size_t file_size = filestat.st_size;
  char buff[buffer_size];

  while (true) {
    FD_ZERO(&wset);
    FD_SET(client_fd, &wset);
    if (select(client_fd + 1, NULL, &wset, NULL, &tv) > 0) {
      nb_read = in.readsome(buff, buffer_size);
      if (in.eof()) {
        break;
      }
      nb_write = send(client_fd, buff, nb_read, 0);

      if (nb_write <= 0) {
        break;
      }
      if (nb_write == nb_read) {
        continue;
      }
      in.seekg(nb_write - nb_read, std::ios::cur);
      // if (nb_read <= 0) break;
      if (in.fail()) {
        break;
      }
    } else {
      break;
    }
  }

  in.close();
}

void Task::response_post(string filename, string command) {
  string file = path;
  file += filename;
  cout << file << endl;
  struct stat filestat;
  int ret = stat(file.c_str(), &filestat);
  if (ret < 0 || S_ISDIR(filestat.st_mode)) {
    string message;
    message += "<html><title>Myhttpd Error</title>";
    message += "<body>\r\n";
    message += " 404\r\n";
    message += " <p>GET: Can't find the file";
    message += " <hr><h3>My Web Server<h3></body>";
    response(message, 404);
    return;
  }
  response_get(filename);
  if (fork() == 0) {
    // Redirect output to the client
    dup2(client_fd, STDOUT_FILENO);
    // Perform subroutines
    execl(file.c_str(), command.c_str(), NULL);
  }
  wait(NULL);
}

void Task::response_head(string filename) {
  string file = path;
  file += filename;

  struct stat filestat;
  int ret = stat(file.c_str(), &filestat);
  if (ret < 0 || S_ISDIR(filestat.st_mode)) {
    string message = "HEAD404";
    response(message, 404);
    return;
  }
  int filefd = open(file.c_str(), O_RDONLY);
  int pos = file.rfind('.', file.length() - 1);
  string suffix = file.substr(pos + 1, file.length() - pos);
  string content_type = get_content_type(suffix);

  response_file(filestat.st_size, 200, content_type);
}

void Task::response_delete(string filename) {
  string file = path;
  file += filename;
  struct stat filestat;
  int ret = stat(file.c_str(), &filestat);
  // cout << ret << endl;
  if (ret < 0) {
    string message;
    message += "<html><title>Myhttpd Error</title>";
    message += "<body>\r\n";
    message += " 404\r\n";
    message += " <p>DELETE: Can't find the file";
    message += "<hr><h3>My Web Server<h3></body>";
    response(message, 404);
    return;
  }
  cout << "get the filename" << file << endl;
  int result = rm(file);
  cout << "The delete result is " << result << endl;
  if (result == 0) {
    string message = "<html>";
    message += "<body>";
    message += "<h1>File deleted.</h1>";
    message += "</body>";
    message += "</html>";
    response(message, 200);
    return;
  }
}

// recursively delete all the file in the directory.
int Task::rm_dir(std::string dir_full_path) {
  DIR* dirp = opendir(dir_full_path.c_str());
  if (!dirp) {
    return -1;
  }
  struct dirent* dir;
  struct stat st;
  while ((dir = readdir(dirp)) != NULL) {
    if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) {
      continue;
    }
    std::string sub_path = dir_full_path + '/' + dir->d_name;
    if (lstat(sub_path.c_str(), &st) == -1) {
      continue;
    }
    if (S_ISDIR(st.st_mode)) {
      if (rm_dir(sub_path) == -1)  // �����Ŀ¼�ļ����ݹ�ɾ��
      {
        closedir(dirp);
        return -1;
      }
      rmdir(sub_path.c_str());
    } else if (S_ISREG(st.st_mode)) {
      unlink(sub_path.c_str());  // �������ͨ�ļ�����unlink
    } else {
      // Log("rm_dir:st_mode ", sub_path, " error");
      continue;
    }
  }
  if (rmdir(dir_full_path.c_str()) == -1)  // delete dir itself.
  {
    closedir(dirp);
    return -1;
  }
  closedir(dirp);
  return 0;
}

int Task::rm(std::string file_name) {
  std::string file_path = file_name;
  struct stat st;
  if (lstat(file_path.c_str(), &st) == -1) {
    return -1;
  }
  if (S_ISREG(st.st_mode)) {
    if (unlink(file_path.c_str()) == -1) {
      return -1;
    }
  } else if (S_ISDIR(st.st_mode)) {
    if (file_name == "." || file_name == "..") {
      return -1;
    }
    if (rm_dir(file_path) == -1)  // delete all the files in dir.
    {
      return -1;
    }
  }
  return 0;
}

string Task::get_content_type(string suffix) {
  if (suffix == "asc" || suffix == "txt" || suffix == "text" ||
      suffix == "pot" || suffix == "brf" || suffix == "srt") {
    return "text/plain";
  }
  if (suffix == "jpeg" || suffix == "jpg") {
    return "image/jpeg";
  }
  if (suffix == "html" || suffix == "htm" || suffix == "shtml") {
    return "text/html";
  }
  if (suffix == "ico") {
    return "image/x-icon";
  }

  return "";
}

#endif
