**IssueNo**:

**Description**:

**通用规范自检：**
| 自检项                                                       | 自检结果  |
| ------------------------------------------------------------ | -------- |
| 避免使用未明确位宽的整型，选择使用int32_t、uint32_t等类型            |<input type="checkbox" unchecked>|
| 统一采用4个空格缩进                                               |<input type="checkbox" unchecked>|
| 成员变量命名，采用xxxx_                                          |<input type="checkbox" unchecked>|
| 系统内创建的线程名以OS_开头，不超过15个字符                         |<input type="checkbox" unchecked>|
| 格式化日志打印注意32/64位兼容，防止64位平台编失败                   |<input type="checkbox" unchecked>|
| 日志内容 同文件内具备唯一性                                        |<input type="checkbox" unchecked>|

**稳定性自检：**
| 自检项                                                       | 自检结果  |
| ------------------------------------------------------------ | -------- |
| 涉及服务端通过ipc回调至客户端的，是否采用异步调用防止服务被卡死   |<input type="checkbox" unchecked>|
| map\vector\list\set等对象存在并发使用时 是否正确加锁            |<input type="checkbox" unchecked>|
| 非必要不使用可重入锁                                            |<input type="checkbox" unchecked>|
| 禁止js对象在非js线程（例如在IPC线程）创建、使用或销毁             |<input type="checkbox" unchecked>|
| 外部传入的指针使用前是否判空                                    |<input type="checkbox" unchecked>|


**安全编码自检：**
| 自检项                                                          | 自检结果 |
| -------------------------------------------------------------- | -------- |
| 反序列化时必须对传入的数组大小进行校验，避免出现超大数组              |<input type="checkbox" unchecked>|
| 新增的成员变量是否正确初始化                                        |<input type="checkbox" unchecked>|
| 有符号、无符号整型 是否正确进行比较防止出现 int32_t(-1) > uint32_t(1) |<input type="checkbox" unchecked>|
| 禁止仅通过TokenType类型判断绕过权限校验                             |<input type="checkbox" unchecked>|
| 指针变量、表示资源描述符的变量、bool变量必须赋初值                  |<input type="checkbox" unchecked>|
| 申请内存后的异常退出处理，是否及时进行内存释放                      |<input type="checkbox" unchecked>|
| 内存申请前是否对内存大小进行合法性校验                              |<input type="checkbox" unchecked>|
| 内存分配后必须判断是否成功                                         |<input type="checkbox" unchecked>|
| 禁止打印内存地址、文件路径、口令等敏感信息，如有需要，使用private修饰   |<input type="checkbox" unchecked>|
| 有符号、无符号整数之间**运算、比较**时必须严格检查，确保不会出现溢出、反转、除0    |<input type="checkbox" unchecked>|

**TDD Result**:

**XTS Result**: