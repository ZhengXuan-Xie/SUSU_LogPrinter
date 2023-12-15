#include "susu_log_printer"


namespace susu_tools{
    
Log_Printer* Log_Printer_handle = NULL;
::std::mutex Log_Printer::init_mutex;

::std::mutex Log_Printer::log_object_init_lock; 


Log_Printer* Log_Printer::get_Log_Printer()
{
    if(Log_Printer_handle == NULL)
    {
        init_mutex.lock();
        if(Log_Printer_handle == NULL)
        {
            Log_Printer_handle = new Log_Printer();
        }
        init_mutex.unlock();
    }
    return Log_Printer_handle;
}


Log_Printer::Log_Printer()
{
    log_path = "./";
}
Log_Printer::~Log_Printer()
{
    thread_log_map.clear();
}

int Log_Printer::print_a_line(int level,::std::string& line)
{
    //忽略低级的log
    if(level < print_level)
    {
        return 0;
    }

    auto time_check = std::chrono::system_clock::now();
    std::time_t nowtime = std::chrono::system_clock::to_time_t(time_check);
    //获取1970年1月1日0点0分0秒到现在经过的秒数
    tm* time_formated = localtime(&nowtime); //将秒数转换为本地时间,年从1900算起,需要+1900,月为0-11,所以要+1
    if(time_formated == NULL)
    {
        fprintf(stderr,"error,can't get local time\n");
        return -1;
    }
    char line_head[256] = {0};
    sprintf(line_head,"\n%d:%d:%d ",time_formated->tm_hour,time_formated->tm_min,time_formated->tm_sec);
    Log_Object& ret = get_thread_object();
    ret.head = line_head;
            ret.log_queue.push(::std::forward<::std::string>(std::move(ret.head.append(line))) );
            if(ret.log_queue.size() >= print_limit) //队列满了，或者遇到高优先级的log，马上执行1次输出。
            {
                write_to_file();

            }
    return 0;
}

int Log_Printer::write_to_file()
{
    auto time_check = std::chrono::system_clock::now();
    std::time_t nowtime = std::chrono::system_clock::to_time_t(time_check);
    //获取1970年1月1日0点0分0秒到现在经过的秒数
    tm* time_formated = localtime(&nowtime); //将秒数转换为本地时间,年从1900算起,需要+1900,月为0-11,所以要+1
    if(time_formated == NULL)
    {
        fprintf(stderr,"error,can't get local time\n");
        return -1;
    }
    char filename[256] = {0};
    sprintf(filename,"%d_%d_%d",time_formated->tm_year + 1900,time_formated->tm_mon + 1,time_formated->tm_mday);
    file = filename;
    //不能free tm*指针，因为这个指针指向静态对象，而不是堆
    //free(time_formated);

    write_mutex.lock();
    ::std::ofstream outputFile(log_path+"/"+file,::std::ios::out|::std::ios::app);//创建且追加写入
    if(outputFile.is_open())
    {
        Log_Object& ret = get_thread_object();
        int i=0;
        while(ret.log_queue.empty() == false/*&& i < limit*/)
        {
            outputFile<<::std::forward<::std::string>(::std::move(ret.log_queue.front()));
            ret.log_queue.pop();
        }
        outputFile.close();
        write_mutex.unlock();
         return 0;
    }
    else
    {
        printf("bad news\n");
        fprintf(stderr,"error,can't write to log file\n");
        write_mutex.unlock();
        return -1;
    }
}

Log_Object& Log_Printer::get_thread_object()
{
    pthread_t thread_id = pthread_self();
    return thread_log_map[thread_id];
}
int Log_Printer::init_thread_object()   //针对每个线程来构造对象
{
    pthread_t thread_id = pthread_self();
    log_object_init_lock.lock();
    if( thread_log_map.find(thread_id) == thread_log_map.end() )
    {
        Log_Object temp;
        thread_log_map[thread_id]= temp;//unordered_map不能重复插入同一个key，需要用[]进行更新。最好也用[]进行插入。
        printf("init a thread_object in %ld\n",thread_id);
    }
    log_object_init_lock.unlock();
    return 0;
}
int Log_Printer::release_thread_object()    //针对每个线程来构造对象
{
    pthread_t thread_id = pthread_self();
    log_object_init_lock.lock();
    if( thread_log_map.find(thread_id) != thread_log_map.end() )
    {
        thread_log_map.erase(thread_id);
        printf("release a thread_object in %ld\n",thread_id);
    }
    log_object_init_lock.unlock();
    return 0;
}
void Log_Printer::print_immediately()
{
    write_to_file();
}
}