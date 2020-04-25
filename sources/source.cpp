// Copyright 2019 dimakirol <your_email>

#include <header.hpp>

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
        target = std::string("/");
        current_depth = 0;
    }
    _download_this(std::string link, std::string path_in_server,
            uint32_t _current_depth, bool _protocol){
        url = link;
        target = path_in_server;
        current_depth = _current_depth;
        protocol = _protocol;
    }
    _download_this(std::string link, std::string path_in_server, uint32_t _current_depth){
        url = link;
        target = path_in_server;
        current_depth = _current_depth;
        protocol = true;
    }
    _download_this(std::string link, uint32_t _current_depth){
        url = link;
        target = std::string("/");
        current_depth = _current_depth;
        protocol = true;
    }
    std::string url;
    std::string target;
    uint32_t current_depth;
    bool protocol;
};
typedef struct _download_this download_this;

struct _parse_this{
    _parse_this(){
        url = std::string("");
        target = std::string("");
        website = std::string("");
        current_depth = 0;
        protocol = false;
    }
    _parse_this(std::string link, std::string path,
                std::string page, uint32_t _current_depth, bool _protocol){
        url = link;
        target = path;
        website = page;
        current_depth = _current_depth;
        protocol = _protocol;
    }
    std::string url;
    std::string target;
    std::string website;
    uint32_t current_depth;
    bool protocol;
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
        bool empty_queue = true;
        while(empty_queue && !finish_him.load()) {
            while (!safe_downloads.try_lock())
                std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 5));
            empty_queue = download_queue->empty();
            safe_downloads.unlock();
        }
        if (finish_him.load())
            return;
//        network_threads->push(std::bind(&MyCrawler::downloading_pages, this, network_threads));

        while (!safe_downloads.try_lock())
            std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 5));
        download_this url_to_download = download_queue->front();
        download_queue->pop();
        safe_downloads.unlock();

        std::string website("");
        if (url_to_download.protocol){
            website = get_https_page(("www." + url_to_download.url),
                                     HTTPS_PORT, url_to_download.target);
            if (website == std::string("404"))
                website = get_https_page(url_to_download.url,
                                         HTTPS_PORT, url_to_download.target);
        }
        else {
            website = get_http_page(("www." + url_to_download.url),
                    HTTP_PORT, url_to_download.target);
        }
        std::cout << website;
        std::cout << std::endl;

        parse_this site(url_to_download.url, website, url_to_download.current_depth);
        while (!safe_processing.try_lock())
            std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 5));
        processing_queue->push(site);
        safe_processing.unlock();
    }
    static void search_for_links(GumboNode* node, 
                                 std::vector<std::string>& img_references, 
                                 std::vector<std::string>& href_references) {
        if (node->type != GUMBO_NODE_ELEMENT) {
            return;
        }
        GumboAttribute* ref;
        if (((node->v.element.tag == GUMBO_TAG_A) || (node->v.element.tag == GUMBO_TAG_LINK)) &&
            (ref = gumbo_get_attribute(&node->v.element.attributes, "href"))) {
            href_references.push_back(std::string(ref->value));
        }

        if (node->v.element.tag == GUMBO_TAG_META &&
            (ref = gumbo_get_attribute(&node->v.element.attributes, "contents"))) {
            img_references.push_back(std::string(ref->value));
        }

        if (node->v.element.tag == GUMBO_TAG_HTML &&
            (ref = gumbo_get_attribute(&node->v.element.attributes, "itemtype"))) {
            href_references.push_back(std::string(ref->value));
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

    void true_site(std::string& site, bool protocol) {
        if (protocol) {
            site = std::string("https://" + site);
        } else {
            site = std::string("http://" + site);
        }
    }
    
    void all_right_references(std::vector<std::string>& img_references,
                          std::vector<std::string>& href_references,
                          std::vector<std::string>& paths_in_hrefs,
                          const std::string& site,
                          const std::string& path_in_site) {
        
        for (auto i = img_references.begin(); i != img_references.end();) {
            if ((i->find(".jpg") != std::string::npos) ||
                (i->find(".png") != std::string::npos) ||
                (i->find(".gif") != std::string::npos) ||
                (i->find(".ico") != std::string::npos) ||
                (i->find(".svg") != std::string::npos)) {
                    if (i->find("/") == 0) {
                        *i = site + *i;
                    }
                    i = img_references.erase(i);
            } else {
                ++i;
            }
        }

        for (auto j = href_references.begin(); j != href_references.end();) {
            if (j->find("#") != std::string::npos) {
                j = href_references.erase(j);
            } else if ((j->find(".jpg") != std::string::npos) ||
                       (j->find(".png") != std::string::npos) ||
                       (j->find(".gif") != std::string::npos) ||
                       (j->find(".svg") != std::string::npos) ||
                       (j->find(".ico") != std::string::npos)) {
                img_references.push_back(*j);
                j = href_references.erase(j);
            } else if (j->find("://") != std::string::npos) {
                if (j->find("/", j->find("://") + 3) != std::string::npos) {
                    paths_in_hrefs.push_back(j->substr(
                            j->find("/", j->find("://") + 3), std::string::npos));
                    *j = j->substr(0, j->find("/", j->find("://") + 3));
                } else {
                    paths_in_hrefs.emplace_back("/");
                }
                ++j;
            } else if (j->find("/") == 0) {
                paths_in_hrefs.push_back(*j);
                *j = site;
                ++j;
            } else {
                ++j;
            }
        }
        
        auto k = href_references.begin();
        for (auto j = paths_in_hrefs.begin(); j != paths_in_hrefs.end();) {
            if (*k == site) {
                if (path_in_site == "/") {
                    *j = j->substr(j->find("/") + 1, std::string::npos);
                }
                *j = path_in_site + *j;
            }
            ++j;
            ++k;
        }
    }
 
    void about_https(std::vector<bool>& https_protocol, std::vector<std::string>& href_references) {
        for (auto j = href_references.begin(); j != href_references.end();) {
            if ((j->find("://") != std::string::npos)) {
                if (j->find("https://") == 0) {
                    *j = j->substr(j->find("://") + 3, std::string::npos);
                    https_protocol.push_back(true);
                } else {
                    *j = j->substr(j->find("://") + 3, std::string::npos);
                    https_protocol.push_back(false);
                }
            }
            if (j->find("www.") != std::string::npos) {
                *j = j->substr(j->find("www.") + 4, std::string::npos);
            }
            ++j;
        }
    }
    
    void parsing_pages(ctpl::thread_pool *parsing_threads) {
        //without finish_him
        //parsing_threads->push(std::bind(&MyCrawler::parsing_pages, this, parsing_threads));
        download_this download_package;
        parse_this parse_package;
        
        while (!safe_processing.try_lock())
            std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 4));
        parse_package = processing_queue->front();
        processing_queue->pop();
        safe_processing.unlock();
        
        std::vector<std::string> img_references;
        std::vector<std::string> href_references;
        std::vector<std::string> paths_in_hrefs;
        std::vector<bool> https_protocol;
        
        GumboOutput* output = gumbo_parse(parse_package.website.c_str());
        search_for_links(output->root, img_references, href_references);
        true_site(parse_package.url, parse_package.protocol);
        all_right_references(img_references, href_references, paths_in_hrefs, parse_package.url, parse_package.target);
        about_https(https_protocol, href_references);
        
        if (parse_package.current_depth) {
            while (!href_references.empty()) {
                download_package.url = href_references[href_references.size() - 1];
                download_package.current_depth = parse_package.current_depth - 1;
                download_package.target = paths_in_hrefs[paths_in_hrefs.size() - 1];
                download_package.protocol = https_protocol[https_protocol.size() - 1];
                while (!safe_downloads.try_lock())
                    std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 4));
                download_queue->push(download_package);
                safe_downloads.unlock();
                href_references.pop_back();
                paths_in_hrefs.pop_back();
                https_protocol.pop_back();
            }
        }

        
        while (!img_references.empty()) {
            while (!safe_output.try_lock())
                std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 4));
            output_queue->push(img_references[img_references.size() - 1]);
            safe_downloads.unlock();
            img_references.pop_back();
        }
        
        gumbo_destroy_output(&kGumboDefaultOptions, output);
    }
    void writing_output(){
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

            //writing_output();

            download_queue->push(download_this(url,(depth - 1)));
            network_threads.push(std::bind(&MyCrawler::downloading_pages, this, &network_threads));

            //parsing_threads.push(std::bind(&MyCrawler::parsing_pages, this, &parsing_threads));
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
