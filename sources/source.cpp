// Copyright 2019 dimakirol <your_email>

#include <header.hpp>
#include <cstdlib>
#include <ctpl.h>
#include <vector>
#include <string>
#include <queue>
#include <mutex>
#include <atomic>
#include <chrono>
#include <fstream>
#include <iostream>
#include <iterator> 
#include <boost/thread/thread.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/beast.hpp>
#include <gumbo.h>


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
    _download_this(){
        url = std::string("");
        current_depth = 0;
    }
    _download_this(std::string link, uint32_t _current_depth){
        url = link;
        current_depth = _current_depth;
    }
    std::string url;
    uint32_t current_depth;
};
typedef struct _download_this download_this;

struct _parse_this{
    _parse_this(){
        website = std::string("");
        current_depth = 0;
    }
    _parse_this(std::string site, uint32_t _current_depth){
        website = site;
        current_depth = _current_depth;
    }
    std::string website;
    uint32_t current_depth;
};
typedef struct _parse_this parse_this;

class MyCrawler{
public:
    explicit MyCrawler(Params &parameters){
        url = parameters.url;
        depth = parameters.depth;
        sites_in_work = 1;
        net_thread = parameters.net_thread;
        pars_thread = parameters.pars_thread;
        out = parameters.out;
        std::cout << url << std::endl << depth << std::endl
        << sites_in_work << std::endl << net_thread << std::endl
        << pars_thread << std::endl << out << std::endl;

        finish_him = false;

        download_queue = new std::queue <download_this>;
        processing_queue = new std::queue <parse_this>;
        output_queue = new std::queue <std::string>;
    }
    ~MyCrawler(){
        delete download_queue;
        delete processing_queue;
        delete output_queue;
    }

private:
    void downloading_pages(ctpl::thread_pool *network_threads){
        std::cout << "Kukuxa uletela" << std::endl;
        //Псевдокод
//        while((!finish_him.load()) && (sites_in_work))
//          while (!safe_downloads.try_lock())
//            std::this_thread::sleep_for(std::chrono::milliseconds(id+1));
//          if (!download_queue->empty()){
//               download_this work_in_process = download_queue.pop();
//               network_threads->push(std::bind(&MyCrawler::downloading_pages, this, network_threads));
//               downloading...
//          }
//          else{
//              std::this_thread::sleep_for(std::chrono::milliseconds(rand()%5));
//          }

    }
    static void search_for_links(GumboNode* node, std::vector<std::string>& img_references, std::vector<std::string>& href_references) {
        if (node->type != GUMBO_NODE_ELEMENT) {
            return;
        }
        GumboAttribute* ref;
        if (node->v.element.tag == GUMBO_TAG_A &&
            (ref = gumbo_get_attribute(&node->v.element.attributes, "href"))) {
            href_references.push_back(std::string(ref->value));
        }

        if (node->v.element.tag == GUMBO_TAG_META &&
            (ref = gumbo_get_attribute(&node->v.element.attributes, "contents"))) {
            img_references.push_back(std::string(ref->value));
        }

        if (node->v.element.tag == GUMBO_TAG_IMG &&
            (ref = gumbo_get_attribute(&node->v.element.attributes, "src"))) {
            img_references.push_back(std::string(ref->value));
        }

        if (node->v.element.tag == GUMBO_TAG_INPUT &&
            (ref = gumbo_get_attribute(&node->v.element.attributes, "type"))) {
            if (std::string(ref->value) == "image") {
                ref = gumbo_get_attribute(&node->v.element.attributes, "src");
                img_references.push_back(std::string(ref->value));
            }
        }

        GumboVector* children = &node->v.element.children;
        for (unsigned int i = 0; i < children->length; ++i) {
            search_for_links(static_cast<GumboNode*>(children->data[i]), img_references, href_references);
        }
    }

    void all_right_img_references(std::vector<std::string>& img_references, std::vector<std::string>& href_references) {
        /*
        for (auto& i = img_references.begin(); i != img_references.end();) {
          if ((i->find(".jpg") == npos) || (i->find(".png") == npos)) {
              i = img_references.erase(i);
          } else {
              ++i;
          }
        }
    */

        for (auto j = href_references.begin(); j != href_references.end();) {
            if ((j->find(".jpg") != std::string::npos) || (j->find(".png") != std::string::npos)) {
                img_references.push_back(*j);
                j = href_references.erase(j);
            } else {
                ++j;
            }
        }
    }
 
    void parsing_pages(ctpl::thread_pool *parsing_threads) {
        //without finish_him
        //parsing_threads->push(std::bind(&MyCrawler::parsing_pages, this, parsing_threads));
        std::string site_to_parse;
        uint32_t current_depth;
        download_this download_package;
        
        while (!safe_processing.try_lock())
            std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 4));
        site_to_parse = processing_queue->front().website;
        current_depth = processing_queue->front().current_depth;
        processing_queue->pop();
        safe_processing.unlock();
        
        std::vector<std::string> img_references;
        std::vector<std::string> href_references;

        GumboOutput* output = gumbo_parse(site_to_parse.c_str());
        search_for_links(output->root, img_references, href_references);
        all_right_img_references(img_references, href_references);
        
        if (current_depth) {
            while (!safe_downloads.try_lock())
                std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 4));
            while (!href_references.empty()) {
                download_package.url = href_references[href_references.size() - 1];
                download_package.current_depth = current_depth - 1;
                download_queue->push(download_package);
                href_references.pop_back();
            }
            safe_downloads.unlock();
        }

        while (!safe_output.try_lock())
            std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 4));
        while (!img_references.empty()) {
            output_queue->push(img_references[img_references.size() - 1]);
            img_references.pop_back();
        }
        safe_downloads.unlock();
        
        gumbo_destroy_output(&kGumboDefaultOptions, output);
    }
    void writing_output(){
        return;
        std::ofstream ostream;
        ostream.open(out, std::ios::out);
        while (!finish_him.load()) {
            if (ostream.is_open()) {
                while (!safe_output.try_lock())
                    std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 5));
                bool empty_queue = output_queue->empty();
                safe_output.unlock();
                while (!empty_queue) {
                    while (!safe_output.try_lock())
                        std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 5));
                    std::string shit_to_write = output_queue->front();
                    ostream << shit_to_write << std::endl;
                    output_queue->pop();
                    empty_queue = output_queue->empty();
                    safe_output.unlock();
                }
                ostream.close();
            } else {
                std::cout << "The file " << out << " is not open" << std::endl;
                throw std::logic_error("Output file is not opened!:(!");
            }
        }
    }

public:
    void crawl_to_live(){
        try {
            ctpl::thread_pool network_threads(net_thread);
            ctpl::thread_pool parsing_threads(pars_thread);

            writing_output();
            download_queue->push(download_this(url, (depth - 1)));
            network_threads.push(std::bind(&MyCrawler::downloading_pages, this, &network_threads));

            parsing_threads.push(std::bind(&MyCrawler::parsing_pages, this, &parsing_threads));
        } catch (std::logic_error const& e){
            std::cout << e.what();
        } catch (...){
            std::cout << "Unknown error! Ask those stupid coders:0";

        }
    }

private:
    std::string url;
    uint32_t depth;
    uint32_t net_thread;
    uint32_t pars_thread;
    std::string out;
    
    std::atomic_bool finish_him;
    uint32_t sites_in_work;

    std::queue <download_this> * download_queue;
    std::mutex safe_downloads;

    std::queue <parse_this> * processing_queue;
    std::mutex safe_processing;

    std::queue <std::string> * output_queue;
    std::mutex safe_output;
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
