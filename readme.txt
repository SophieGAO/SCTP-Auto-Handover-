*************************************************************
Server side program:
*************************************************************
1.Function
Server is setting up and waiting for client request. And it will send requested file to client. The handover process is implemented when a cable is connected to client when the program is running.

2.Structure
The program will extecute necessary communiation with client when the client sent request to it.
Then the main structure includes three parts which are implemented by pthread.
**************************************************************
Thread Name/ Thread ID/ Thread Function/ Running Status
**************************************************************
Thread Name:File_Transfer	
Thread ID:tid1	
Thread Function:This thread is used to implement the file transfer function. It opens the requested file by client and using sctp_sendmsg( ) to transfer the file to client.
Running Status:Running all the time until the file transfer finished
**************************************************************
Thread Name:changePrimary	
Thread ID:tid2	
Thread Function:This thread set up the new client IP when getting the signal from thread ¡®checklink¡¯.
Running Status:Locked until triggered by signal from thread ¡®checklink¡¯
**************************************************************
Thread Name:checklink	
Thread ID:linkid	
Thread Function:This thread is used to check whether the client is starting to change primary client IP address. If a message is received from client checked, it will send a signal to trigger the thread ¡®changePrimary¡¯.
Running Status:Loop until receive the message recvSig[] sent from client  side and equal to changeIPSig[].
**************************************************************

3.Compile
Compile server program in terminal using command "gcc -lsctp -lpthread -o s s.c"

4.Execution
Using command "./s" to run server program.
*************************************************************
*************************************************************

*************************************************************
Client side program:
*************************************************************
1.Function
Client is connecting to server and send requested file name.Then it will download the requested file from server. The handover process is implemented when a cable is connected to client when the program is running.

2.Structure
The program will extecute necessary communiation with server when connect to server and sent requested file name.
Then the main structure includes three parts which are implemented by pthread.
**************************************************************
Thread Name/ Thread ID/ Thread Function/ Running Status
**************************************************************
Thread Name:File_Transfer	
Thread ID:tid1	
Thread Function:This thread is used to implement the file transfer function. It receive the requested file by using recv_sendmsg( ). Then it will create a new file and write in the received message. 
Running Status:Running all the time until the file transfer finished
**************************************************************
Thread Name:changePrimary	
Thread ID:tid2	
Thread Function:This thread changes the primary IP to the new client IP when getting the signal from thread ¡®checklink¡¯. It changes primary address by using the important parameter ¡®assoc_id¡¯ and setsockopt(SCTP_SET_PEER_PRIMARY_ADDR). 
Running Status:Locked until triggered by signal from thread ¡®checklink¡¯
**************************************************************
Thread Name:checklink	
Thread ID:linkid	
Thread Function:This thread is used to check whether the client is starting to change primary client IP address. If a message is received from client checked, it will send a signal to trigger the thread ¡®changePrimary¡¯.
Running Status:Loop until a cable connection is detected when get_netlink_status()=1.
**************************************************************

3.Compile
Compile server program in terminal using command "gcc -lsctp -lpthread -o c c.c"

4.Execution
Using command "./c" to run server program.