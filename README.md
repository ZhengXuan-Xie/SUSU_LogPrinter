# SUSU_LogPrinter
A simple C++11 Log Printer. It's friendly with multi thread.

# how to use
int a thread

    Log_Printer* log = Log_Printer::get_Log_Printer(); // init global object 
    log->set_path("./");  //set log file path
    log->set_print_level(1);  //set print level.if level is lower than this value,the log will be ignored.
    
    log->init_thread_object();    //insert a thread object in global object
    log->print_a_line(2,"hello world");     //print a line,but not write immediately. log object will write to file every 32 lines log
    log->print_immediately();  //print and write immediately
