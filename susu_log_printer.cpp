#include "susu_log_printer"


namespace susu_tools{
    
Log_Printer* Log_Printer_handle = NULL;
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

::std::mutex Log_Printer::init_mutex;

Log_Printer::Log_Printer()
{
    log_path = "./";
    lock = 0;//表示可占用
    //log_write_thread = std::thread([this](){this->work_loop();});
}
Log_Printer::~Log_Printer()
{
    //write_to_file();
    log_map.clear();
}

int Log_Printer::print_a_line(int level,::std::string& line)
{
    //忽略低级的log
    if(level < print_level)
    {
        return 0;
    }

    /*time_t nowtime = time(0);
	time(&nowtime); */
    auto time_check = std::chrono::system_clock::now();
    std::time_t nowtime = std::chrono::system_clock::to_time_t(time_check);
    //获取1970年1月1日0点0分0秒到现在经过的秒数
    tm* time_formated = localtime(&nowtime); //将秒数转换为本地时间,年从1900算起,需要+1900,月为0-11,所以要+1
	//printf("%04d:%02d:%02d %02d:%02d:%02d\n", time_formated->tm_year + 1900, time_formated->tm_mon + 1, time_formated->tm_mday,time_formated->tm_hour,time_formated->tm_min,time_formated->tm_sec);
    if(time_formated == NULL)
    {
        fprintf(stderr,"error,can't get local time\n");
        return -1;
    }
    char line_head[256] = {0};
    //sprintf(line_head,"%d-%d-%d %d:%d:%d ",time_formated->tm_year+1800,time_formated->tm_mon+1,time_formated->tm_mday,time_formated->tm_hour,time_formated->tm_min,time_formated->tm_sec);
    sprintf(line_head,"\n%d:%d:%d ",time_formated->tm_hour,time_formated->tm_min,time_formated->tm_sec);

    //这里不能用mutex，而是应该使用自旋锁
    //add_mutex.lock();
    //head = line_head;
    Log_Object& ret = get_thread_object();
    int temp_test = get_thread_int();
    ret.head = line_head;
    //while(true)//自旋锁 
    //{
        //if(lock == 0)
        //{
            //lock = 1;
            ret.log_queue.push(::std::forward<::std::string>(std::move(ret.head.append(line))) );//head + " " +line是一个临时对象,这里分配了内存。现在要考虑的是，能否在push的时候不新建对象？而是使用右值引用。
            //log_queue.push( head + " " +line );
            
            if(ret.log_queue.size() >= print_limit || level >= immediately_print_level) //队列满了，或者遇到高优先级的log，马上执行1次输出。
            {
                write_to_file();
                //printf("now the int poll size is %d and the int size is %d\n",log_map.size(),test_map.size());
                //write_to_file();
                //printf("add a task,now the size is %d\n",ret.log_queue.size());
                //queue<string> temp;
                //for(int i=0;i<print_limit;i++)
                //{
                //    temp.push(::std::forward<::std::string>(::std::move(ret.log_queue.front())));
                //    ret.log_queue.pop();
                //}
                //Log_Object sub_log_queue;
                //add_a_task(temp);
            }
            //lock = 0;
            //break;
        //}
        //else
        //{
        //    ::std::this_thread::sleep_for(::std::chrono::milliseconds(5));
        //    continue;
        //}
    //}
    //add_mutex.unlock();
    return 0;
}
int Log_Printer::set_path(::std::string path)
{
    log_path = path;
    return 0;
}

//int Log_Printer::write_to_file(Log_Object& log_print_object,int limit)
int Log_Printer::write_to_file()
{
    /*time_t nowtime = time(0);
	time(&nowtime); */
    auto time_check = std::chrono::system_clock::now();
    std::time_t nowtime = std::chrono::system_clock::to_time_t(time_check);
    //获取1970年1月1日0点0分0秒到现在经过的秒数
    tm* time_formated = localtime(&nowtime); //将秒数转换为本地时间,年从1900算起,需要+1900,月为0-11,所以要+1
	//printf("%04d:%02d:%02d %02d:%02d:%02d\n", time_formated->tm_year + 1900, time_formated->tm_mon + 1, time_formated->tm_mday,time_formated->tm_hour,time_formated->tm_min,time_formated->tm_sec);
    if(time_formated == NULL)
    {
        fprintf(stderr,"error,can't get local time\n");
        return -1;
    }
    char filename[256] = {0};
    sprintf(filename,"%d_%d_%d",time_formated->tm_year + 1900,time_formated->tm_mon + 1,time_formated->tm_mday);
    //printf("write date %s\n",filename);
    file = filename;
    //printf("now the date is %s \n",filename);
    //不能free tm*指针，因为这个指针指向静态对象，而不是堆
    //free(time_formated);

    write_mutex.lock();
    ::std::ofstream outputFile(log_path+"/"+file,::std::ios::out|::std::ios::app);//创建且追加写入
    if(outputFile.is_open())
    {
        Log_Object& ret = get_thread_object();
        int temp_test = get_thread_int();
        int i=0;
        while(ret.log_queue.empty() == false/*&& i < limit*/)
        {
            //printf("the limit is %d\n",limit);
            //outputFile<<log_queue.front() << "\n";
            outputFile<<::std::forward<::std::string>(::std::move(ret.log_queue.front()));
            ret.log_queue.pop();
            //i++;
            //printf("the i is %d\n",i);
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
    //std::thread::id thread_id = std::this_thread::get_id();
    pthread_t thread_id = pthread_self();
    if( log_map.find(thread_id) == log_map.end() )
    {
        //注意，由于线程id具有唯一性，所以不存在2个线程同时插入1个 pid 的情况，此处无需使用双锁
        log_object_init_lock.lock();
        /*if( log_map.find(thread_id) == log_map.end() )
        {*/
            Log_Object temp;
            //log_map.insert(std::pair<std::thread::id,Log_Object>(thread_id,temp));
            log_map.insert(std::pair<pthread_t,Log_Object>(thread_id,temp));
        /*}*/
        log_object_init_lock.unlock();
    }
    return log_map[thread_id];
}
void Log_Printer::print_immediately()
{
    write_to_file();
}

void Log_Printer::add_a_task(Log_Object&& ret)
{
    //printf("add a task\n");
    //Log_Object& ret = get_thread_object();
    //add_mutex.lock();
    //task_queue.push(
                        //[this,ret]
                        //()
                        //{ 
                        //    Log_Object temp = ret;
                            //Log_Object temp = ret;
                            //this->write_to_file_async(std::forward<Log_Object>(std::move(const_cast<Log_Object>(ret))));
                        //    this->write_to_file_async(std::forward<Log_Object>(std::move(temp)));
                        //}
                    //);
    //add_mutex.unlock();
    //printf("now the task queue count is %d\n",task_queue.size());
}



void Log_Printer::work_loop()
{
    while(true)
    {
        if(log_map.size() == 0)
        {
            ::std::this_thread::sleep_for(::std::chrono::milliseconds(500));
        }
        else
        {
            for(auto it = log_map.begin();it != log_map.end();it++)
            {
                //printf("now the task count is %d\n",it->second.task_queue.size());
                if(it->second.task_queue.empty())
                {
                    continue;
                }
        
                //printf("do somethings\n");
                //get and run a task. the result will be store in future queue,so some another object can get this result by future
                ::std::function<void(queue<string>&&)> task = ::std::move(it->second.task_queue.front());
                it->second.task_queue.pop();
                std::queue<string> temp = ::std::move(it->second.log_part.front());
                it->second.log_part.pop();
                task(std::forward<queue<string>>(std::move(temp)));
            }
        }
        
    }
}

int Log_Printer::print_a_line_async(int level,::std::string& line)
{
    //忽略低级的log
    if(level < print_level)
    {
        return 0;
    }

    /*time_t nowtime = time(0);
	time(&nowtime); */
    auto time_check = std::chrono::system_clock::now();
    std::time_t nowtime = std::chrono::system_clock::to_time_t(time_check);
    //获取1970年1月1日0点0分0秒到现在经过的秒数
    tm* time_formated = localtime(&nowtime); //将秒数转换为本地时间,年从1900算起,需要+1900,月为0-11,所以要+1
	//printf("%04d:%02d:%02d %02d:%02d:%02d\n", time_formated->tm_year + 1900, time_formated->tm_mon + 1, time_formated->tm_mday,time_formated->tm_hour,time_formated->tm_min,time_formated->tm_sec);
    if(time_formated == NULL)
    {
        fprintf(stderr,"error,can't get local time\n");
        return -1;
    }
    char line_head[256] = {0};
    //sprintf(line_head,"%d-%d-%d %d:%d:%d ",time_formated->tm_year+1800,time_formated->tm_mon+1,time_formated->tm_mday,time_formated->tm_hour,time_formated->tm_min,time_formated->tm_sec);
    sprintf(line_head,"\n%d:%d:%d ",time_formated->tm_hour,time_formated->tm_min,time_formated->tm_sec);

    Log_Object& ret = get_thread_object();
    int temp_test = get_thread_int();
    ret.head = line_head;
            ret.log_queue.push(::std::forward<::std::string>(std::move(ret.head.append(line))) );//head + " " +line是一个临时对象,这里分配了内存。现在要考虑的是，能否在push的时候不新建对象？而是使用右值引用。
            if(ret.log_queue.size() >= print_limit) //队列满了，或者遇到高优先级的log，马上执行1次输出。
            {
                /*以交换内存的形式，复制当前的Log队列，作为打印任务的一部分。原有对象的Log队列将被清空。
                但请注意，打印任务会被放在原有对象的任务队列之内，而不是放到临时对象内*/

                queue<string> temp;
                std::swap(temp,ret.log_queue);
                ret.log_part.push(std::forward<queue<string>>(std::move(temp)));
                ret.task_queue.push(
                        [this]
                        (queue<string>&& print_object)
                        {
                            //std::swap(temp.log_queue,print_object.log_queue);
                            //printf("the print_object size is %d\n,the temp log_queue size is %d\n",print_object.log_queue.size(),temp.log_queue.size());
                            this->write_to_file_async(std::forward<queue<string>>(std::move(print_object)));
                        }
                    );
            }
    return 0;
}

int Log_Printer::write_to_file_async(queue<string>&& log_print_object)
{
    /*time_t nowtime = time(0);
	time(&nowtime); */
    auto time_check = std::chrono::system_clock::now();
    std::time_t nowtime = std::chrono::system_clock::to_time_t(time_check);
    //获取1970年1月1日0点0分0秒到现在经过的秒数
    tm* time_formated = localtime(&nowtime); //将秒数转换为本地时间,年从1900算起,需要+1900,月为0-11,所以要+1
	//printf("%04d:%02d:%02d %02d:%02d:%02d\n", time_formated->tm_year + 1900, time_formated->tm_mon + 1, time_formated->tm_mday,time_formated->tm_hour,time_formated->tm_min,time_formated->tm_sec);
    if(time_formated == NULL)
    {
        fprintf(stderr,"error,can't get local time\n");
        return -1;
    }
    char filename[256] = {0};
    sprintf(filename,"%d_%d_%d",time_formated->tm_year + 1800,time_formated->tm_mon + 1,time_formated->tm_mday);
    file = filename;
    //不能free tm*指针，因为这个指针指向静态对象，而不是堆
    //free(time_formated);

    //write_mutex.lock();
    ::std::ofstream outputFile(log_path+"/"+file,::std::ios::out|::std::ios::app);//创建且追加写入
    if(outputFile.is_open())
    {
        while(log_print_object.empty() == false/*&& i < limit*/)
        {
            //printf("the limit is %d\n",limit);
            //outputFile<<log_queue.front() << "\n";
            outputFile<<::std::forward<::std::string>(::std::move(log_print_object.front()));
            log_print_object.pop();
            //i++;
            //printf("the i is %d\n",i);
        }
        outputFile.close();
        //write_mutex.unlock();
    }
    else
    {
        printf("bad news\n");
        fprintf(stderr,"error,can't write to log file\n");
        //write_mutex.unlock();
        return -1;
    }
    
    return 0;
}
int Log_Printer::get_thread_int()
{
    //std::thread::id thread_id = std::this_thread::get_id();
    pthread_t thread_id = pthread_self();
    if( test_map.find(thread_id) == test_map.end() )
    {
        write_mutex.lock();
        test_map.insert(std::pair<pthread_t,int>(thread_id,1));
        //printf("you build a int in thread id %lu\n",thread_id);
        puts("you build a int in thread id");
        write_mutex.unlock();
    }
    return test_map[thread_id];
}
}