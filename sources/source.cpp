// Copyright 2019 dimakirol <your_email>

#include <header.hpp>
#include <ctpl.h>
#include <queue>
#include <mutex>
#include <chrono>
#include <fstream>
#include <iostream>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

namespace po=boost::program_options;

struct _Params{
    std::string url;
    uint32_t depth;
    uint32_t net_thread;
    uint32_t pars_thread;
    std::string out;
};
typedef struct _Params Params;

struct _download_this{
    std::string url;
    uint32_t current_depth;
};
typedef struct _download_this download_this;

struct _parse_this{
    ifstream website_file;
    uint32_t current_depth;
};
typedef struct _parse_this parse_this;

class MyCrawler{
public:
    MyCrawler(Params &parameters){
        url = parameters.url;
        depth = parameters.depth;
        net_thread = parameters.net_thread;
        pars_thread = parameters.pars_thread;
        out = parameters.out;
        
        finish_him = false;

        download_queue = new std::queue <download_this>;
        processing_queue = new std::queue <parse_this>;
        output_queue = new std::queue <std::string>;

        *network_threads = ctpl::thread_pool(net_thread);
        *parsing_threads = ctpl::thread_pool(pars_thread);
    }
    ~MyCrawler(){
        delete download_queue;
        delete processing_queue;
        delete output_queue;
    }

private:
    void downloading_pages(int id){
        /*Псевдокод
         * if (!download_queue->empty()){
         *      download_this work_in_process = download_queue.pop();
         *      network_threads->push(downloading_pages);
         *      downloading...
         * }
         * else{
         *      sleep 1 second;
         * }
        */
    }
    void parsing_pages(){
        // см downloading чтоб понять осн суть работы с пулом потоков
    }
    void writing_output(){
        while (!safe_output.try_lock())
            std::this_thread::sleep_for(std::chrono::milliseconds(id+1));
        ofstream ostream;
        ostream.open("output.txt", ios::out);
        if (ostream.is_open()){
            while(!output_queue.empty()){
                std::string shit_to_write = output_queue.front();
                ostream << shit_to_write << endl;
                output_queue.pop();
            }
            ostream.close();
        }
        else{
            cout << "The file 'output.txt' is not open" << endl;
        }
        safe_output.unlock();
    }

public:
    void crawl_to_live(){
        download_queue->push(download_this(url, (depth - 1)));
        network_threads->push(downloading_pages);

        parsing_threads->push(parsing_pages);

        writing_output();
    }

private:
    std::string url;
    uint32_t depth;
    uint32_t net_thread;
    uint32_t pars_thread;
    std::string out;
    
    bool finish_him;

    std::queue <download_this> * download_queue;
    std::mutex safe_downloads;

    std::queue <parse_this> * processing_queue;
    std::mutex safe_processing;

    std::queue <std::string> * output_queue;
    std::mutex safe_output;

    ctpl::thread_pool * network_threads;
    ctpl::thread_pool * parsing_threads;
};

Params parse_cmd(const po::variables_map& vm){
    Params cmd_params;
    if (vm.count("url"))
        cmd_params.url = vm["url"].as<std::string>();
    if(vm.count("depth"))
        cmd_params.depth = vm["depth"].as<uint32_t>();
    if(vm.count("network_threads"))
        cmd_params.net_thread = vm["network_threads"].as<uint32_t>();
    if(vm.count("parser_threads"))
        cmd_params.pars_thread = vm["parser_threads"].as<uint32_t>();
    if(vm.count("output"))
        cmd_params.out = vm["output"].as<std::string>();
    return cmd_params;
}

int main(int argc, char* argv[]){
    po::options_description desc("General options");
    std::string task_type;
    desc.add_options()
            ("help,h", "Show help")
            ("type,t", po::value<std::string>(&task_type),"Select task: parse")
            ;
    po::options_description parse_desc("Work options (everything required)");
    parse_desc.add_options()
            ("url,u", po::value<std::string>(), "Input start page")
            ("depth,d", po::value<uint32_t>(), "Input depth")
            ("network_threads,N", po::value<uint32_t>(), "Input number of downloading threads")
            ("parser_threads,P", po::value<uint32_t>(), "Input number of parsing threads")
            ("output,O", po::value<std::string>(), "Output parameters file")
            ;
    po::variables_map vm;
    try {
        po::parsed_options parsed = po::command_line_parser(argc, argv).options(desc).allow_unregistered().run();
        po::store(parsed, vm);
        po::notify(vm);
        if(task_type == "parse") {
            desc.add(parse_desc);
            po::store(po::parse_command_line(argc, argv, desc), vm);
            Params cmd_params = parse_cmd(vm);

//..............................................................................
            MyCrawler crawler(cmd_params);
            crawler.crawl_to_live();
//..............................................................................
        }
        else {
            desc.add(parse_desc);
            std::cout << desc << std::endl;
        }
    }
    catch(std::exception& ex) {
        std::cout << desc << std::endl;
    }
    return 0;
}
