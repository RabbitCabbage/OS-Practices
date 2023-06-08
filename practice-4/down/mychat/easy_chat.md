# EASY CHAT of RabbitCabbage
1. 通讯录
    a.每个client接进来必须要先给自己取个名字，并且有个密码，只能是英文字母、下划线、数字组成，不能有空格，不能重复，server会检查这个名字的合法性。名字和密码都不能超过10个字符。会在文件中有记录，
    b.每个client接进来后，server会把这个client的名字存到map里面，表示已经登录。如果这个client断开连接，server会把这个client的名字从map里面删除，表示已经下线。
    c.某个client可以通过输入`list`命令来查看当前在线的client，server会把当前在线的client的名字发给这个client。
    d.某个client可以把另外一个client加入到自己的通讯录中，通过输入`add <name>`，在对方同意后就可以把互相对方加入到自己的通讯录中。
    e.某个client可以把另外一个client从自己的通讯录中删除，通过输入`del <name>`，就可以互相把对方从自己的通讯录中删除。
    f.某个client可以通过输入`send <name> message`来给另外一个client发送消息，如果对方在线，就直接把消息发给对方，如果对方不在线，就把消息存入数据库中，等对方上线后再把消息发给对方。
    g.某个client可以通过输入`addr`命令来查看自己的通讯录，server会把通讯录发给这个client。

2. 聊天记录
    a.某个client可以通过输入`history <name>`来查看和某个client的聊天记录，server会把聊天记录发给这个client。
    b.某个client可以通过输入`history`来查看自己的聊天记录，server会把聊天记录发给这个client。
    c.某个client可以通过输入`clear <name>`来清除和某个client的聊天记录，server会把聊天记录清除。
    d. server在log文件夹下为每个client维护一个聊天记录的文件夹，文件夹名字是client的名字，文件夹下面有多个文件，文件名字是聊天的对象的名字，文件内容是聊天记录，每次聊天记录都会追加到文件中。读出时全部读出。

3. 安全性
    a. DDos攻击: server会记录每个client的ip地址，如果某个client在短时间内发送大量的请求，server会把这个client的ip地址加入到黑名单中，server不再接受这个client的请求。
    b. 某个client可以通过输入`login <name> <password>`来登录，server会检查这个client的名字和密码是否正确，如果正确，就把这个client的登录状态设置为已登录，如果不正确，就把这个client的登录状态设置为未登录。