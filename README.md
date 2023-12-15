# SUSU_LogPrinter
A simple C++11 Log Printer. It's friendly with multi thread.

# how to use
int a thread

    Log_Printer* log = Log_Printer::get_Log_Printer(); // init global object 
    log->set_path(init_param->get_string_value("log_path"));  //set log file path
    log->set_print_level(init_param->get_int_value("log_level"));  //set print level
    
    log->init_thread_object();//构造对象
    log->print_a_line(2,temp_log_str);     //print a line,but not write immediately
    log->print_immediately();  //print and write immediately
