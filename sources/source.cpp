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
        protocol = true;
    }
    _download_this(std::string link, std::string path_in_server,
            uint32_t _current_depth, bool _protocol){
        url = link;
        target = path_in_server;
        current_depth = _current_depth;
        protocol = _protocol;
    }
    _download_this(std::string link, std::string path_in_server,
                   uint32_t _current_depth){
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
    bool protocol; //0-http;  1-https
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
        sites_in_work.store(odin);
        net_thread = parameters.net_thread;
        pars_thread = parameters.pars_thread;
        out = parameters.out;

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
        bool empty_queue = true;
        while (empty_queue && !finish_him.load()) {
            while (!safe_downloads.try_lock()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(
                        kirill_sleeps_seconds));
            }
            empty_queue = download_queue->empty();
            safe_downloads.unlock();
        }
        if (finish_him.load()) {
            return;
        }

        while (!safe_downloads.try_lock()){
            std::this_thread::sleep_for(std::chrono::milliseconds(
                    kirill_sleeps_seconds));
        }
        download_this url_to_download = download_queue->front();
        download_queue->pop();
        safe_downloads.unlock();

        network_threads->push(std::bind(&MyCrawler::downloading_pages,
                this, network_threads));

        std::mutex down_load;
        std::string website("");
        down_load.lock();
        if (url_to_download.protocol){
            website = get_https_page(url_to_download.url,
                                     HTTPS_PORT, url_to_download.target);
            if ((website == std::string(NOT_FOUND)) ||
                (website == std::string("")) ||
                (website.find(MOVED_PERMANENTLY) != std::string::npos)) {
                        website = get_https_page((WWW + url_to_download.url),
                                         HTTPS_PORT, url_to_download.target);
            }
        } else {
            website = get_http_page((WWW + url_to_download.url),
                    HTTP_PORT, url_to_download.target);
        }
        down_load.unlock();
        parse_this site(url_to_download.url, url_to_download.target,
                website, url_to_download.current_depth,
                url_to_download.protocol);
        while (!safe_processing.try_lock()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(
                    kirill_sleeps_seconds));
        }
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
        if (((node->v.element.tag == GUMBO_TAG_A) ||
            (node->v.element.tag == GUMBO_TAG_LINK)) &&
            (ref = gumbo_get_attribute(&node->v.element.attributes, HREF))){
            href_references.push_back(std::string(ref->value));
        }

        if (node->v.element.tag == GUMBO_TAG_META &&
            (ref = gumbo_get_attribute(&node->v.element.attributes,
                                       CONTENTS))) {
            img_references.push_back(std::string(ref->value));
        }

        if (node->v.element.tag == GUMBO_TAG_HTML &&
            (ref = gumbo_get_attribute(&node->v.element.attributes,
                                       ITEMTYPE))) {
            href_references.push_back(std::string(ref->value));
        }

        if (node->v.element.tag == GUMBO_TAG_IMG &&
            (ref = gumbo_get_attribute(&node->v.element.attributes, SRC))) {
            img_references.push_back(std::string(ref->value));
        }

        if (node->v.element.tag == GUMBO_TAG_INPUT &&
            (ref = gumbo_get_attribute(&node->v.element.attributes, TYPE))){
            if (std::string(ref->value) == IMAGE) {
                ref = gumbo_get_attribute(&node->v.element.attributes, SRC);
                img_references.push_back(std::string(ref->value));
            }
        }

        GumboVector* children = &node->v.element.children;
        for (unsigned int i = 0; i < children->length; ++i) {
            search_for_links(static_cast<GumboNode*>(children->data[i]),
                    img_references, href_references);
        }
    }

    void true_site(std::string& site, bool protocol) {
        if (protocol) {
            site = std::string(HTTPS + site);
        } else {
            site = std::string(HTTP + site);
        }
    }

    void all_right_references(std::vector<std::string>& img_references,
                          std::vector<std::string>& href_references,
                          std::vector<std::string>& paths_in_hrefs,
                          const std::string& site,
                          const std::string& path_in_site) {
        for (auto i = img_references.begin(); i != img_references.end();) {
            if ((i->find(JPG) != std::string::npos) ||
                (i->find(PNG) != std::string::npos) ||
                (i->find(GIF) != std::string::npos) ||
                (i->find(ICO) != std::string::npos) ||
                (i->find(SVG) != std::string::npos)) {
                    if (i->find("/") == 0) {
                        *i = site + *i;
                    }
                    ++i;
            } else {
                i = img_references.erase(i);
            }
        }

        for (auto j = href_references.begin(); j != href_references.end();) {
            if (j->find(OCTOTORP) != std::string::npos) {
                j = href_references.erase(j);
            } else if ((j->find(JPG) != std::string::npos) ||
                       (j->find(PNG) != std::string::npos) ||
                       (j->find(GIF) != std::string::npos) ||
                       (j->find(ICO) != std::string::npos) ||
                       (j->find(SVG) != std::string::npos)) {
                if (j->find("/") == 0) {
                    *j = site + *j;
                }
                img_references.push_back(*j);
                j = href_references.erase(j);
            } else if (j->find(SAD_SMILE) != std::string::npos) {
                if (j->find("/", j->find(SAD_SMILE) + tri) != std::string::npos) {
                    paths_in_hrefs.push_back(j->substr(
                            j->find("/", j->find(SAD_SMILE) + tri),
                                          std::string::npos));
                    *j = j->substr(0, j->find("/", j->find(SAD_SMILE) + tri));
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
                    *j = j->substr(j->find("/") + odin, std::string::npos);
                }
                *j = path_in_site + *j;
            }
            ++j;
            ++k;
        }
    }

    void about_https(std::vector<bool>& https_protocol,
            std::vector<std::string>& href_references) {
        for (auto j = href_references.begin(); j != href_references.end();) {
            if ((j->find(SAD_SMILE) != std::string::npos)) {
                if (j->find(HTTPS) == 0) {
                    *j = j->substr(j->find(SAD_SMILE) + tri, std::string::npos);
                    https_protocol.push_back(true);
                } else {
                    *j = j->substr(j->find(SAD_SMILE) + tri, std::string::npos);
                    https_protocol.push_back(false);
                }
            }
            if (j->find(WWW) != std::string::npos) {
                *j = j->substr(j->find(WWW) + chetire, std::string::npos);
            }
            ++j;
        }
    }

    void parsing_pages(ctpl::thread_pool *parsing_threads) {
        bool empty_queue = true;
        while (empty_queue && !finish_him.load()) {
            while (!safe_processing.try_lock()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(
                        dima_sleeps_seconds));
            }
            empty_queue = processing_queue->empty();
            safe_processing.unlock();
        }
        if (finish_him.load()) {
            return;
        }

        download_this download_package;
        parse_this parse_package;

        while (!safe_processing.try_lock()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(
                    dima_sleeps_seconds));
        }
        parse_package = processing_queue->front();
        processing_queue->pop();
        safe_processing.unlock();

        parsing_threads->push(std::bind(&MyCrawler::parsing_pages,
                              this, parsing_threads));

        std::vector<std::string> img_references;
        std::vector<std::string> href_references;
        std::vector<std::string> paths_in_hrefs;
        std::vector<bool> https_protocol;

        GumboOutput* output = gumbo_parse(parse_package.website.c_str());
        search_for_links(output->root, img_references, href_references);
        true_site(parse_package.url, parse_package.protocol);
        all_right_references(img_references, href_references,
                             paths_in_hrefs, parse_package.url,
                                 parse_package.target);
        about_https(https_protocol, href_references);

        if (parse_package.current_depth) {
            if (!href_references.empty()) {
                bool first_one = true;
                while (!href_references.empty()) {
                    download_package.url = href_references[
                                                href_references.size() - odin];
                    download_package.current_depth =
                            parse_package.current_depth - odin;
                    download_package.target = paths_in_hrefs[
                                                 paths_in_hrefs.size() - odin];
                    download_package.protocol = https_protocol[
                                                 https_protocol.size() - odin];
                    while (!safe_downloads.try_lock()) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(
                                dima_sleeps_seconds));
                    }
                    download_queue->push(download_package);
                    safe_downloads.unlock();
                    href_references.pop_back();
                    paths_in_hrefs.pop_back();
                    https_protocol.pop_back();
                    if (!first_one){
                        sites_in_work.store(sites_in_work.load() + odin);
                    } else {
                        first_one = false;
                    }
                }
            } else {
                sites_in_work.store(sites_in_work.load() - odin);
            }
        } else {
            sites_in_work.store(sites_in_work.load() - odin);
        }
        std::cout << "Sites in work " << sites_in_work.load()
                  << ".............." << std::endl;

        while (!img_references.empty()) {
            while (!safe_output.try_lock()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(
                        dima_sleeps_seconds));
            }
            output_queue->push(img_references[img_references.size() - odin]);
            safe_output.unlock();
            img_references.pop_back();
        }
        gumbo_destroy_output(&kGumboDefaultOptions, output);

        if (!sites_in_work.load()) {
            finish_him.store(true);
        }
    }
    void writing_output(){
        std::ofstream ostream;
        ostream.open(out);
        if (!ostream.is_open()){
            std::cout << "The file " << out << " is not open" << std::endl;
            throw std::logic_error("Output file is not opened!:(! ");
        }
        while (!finish_him.load()) {
            while (!safe_output.try_lock()){
                std::this_thread::sleep_for(std::chrono::milliseconds(
                        masha_sleeps_seconds));
            }
            bool empty_queue = output_queue->empty();
            safe_output.unlock();
            while (!empty_queue) {
                while (!safe_output.try_lock()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(
                            masha_sleeps_seconds));
                }
                std::string shit_to_write = output_queue->front();
                ostream << shit_to_write << std::endl;
                output_queue->pop();
                empty_queue = output_queue->empty();
                safe_output.unlock();
            }
        }
        ostream.close();
    }
    std::string prepare_url(std::string &url){
        std::string target("");
        if (url.find("/") != std::string::npos){
            target = url.substr(url.find("/"), std::string::npos);
            url.assign(url, 0, url.find("/"));
        } else {
            target = "/";
        }
        return target;
    }

public:
    void crawl_to_live(){
        try {
            ctpl::thread_pool network_threads(net_thread);
            ctpl::thread_pool parsing_threads(pars_thread);

            std::string target = prepare_url(url);
            download_queue->push(download_this(url, target, depth));

            network_threads.push(std::bind(&MyCrawler::downloading_pages,
                            this, &network_threads));
            parsing_threads.push(std::bind(&MyCrawler::parsing_pages,
                                this, &parsing_threads));
            writing_output();
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
    std::atomic_uint sites_in_work;

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
    if (vm.count("depth"))
        cmd_params.depth = vm["depth"].as<uint32_t>();
    if (vm.count("network_threads"))
        cmd_params.net_thread = vm["network_threads"].as<uint32_t>();
    if (vm.count("parser_threads"))
        cmd_params.pars_thread = vm["parser_threads"].as<uint32_t>();
    if (vm.count("output"))
        cmd_params.out = vm["output"].as<std::string>();
    return cmd_params;
}
Params command_line_processor(int argc, char* argv[]){
	po::options_description desc("General options");
	std::string task_type;
	desc.add_options()
			("help,h", "Show help")
			("type,t", po::value<std::string>(&task_type),
			 "Select task: parse");
	po::options_description parse_desc("Work options (everything required)");
	parse_desc.add_options()
			("url,u", po::value<std::string>(), "Input start page")
			("depth,d", po::value<uint32_t>(), "Input depth")
			("network_threads,N", po::value<uint32_t>(),
			 "Input number of downloading threads")
			("parser_threads,P", po::value<uint32_t>(),
			 "Input number of parsing threads")
			("output,O", po::value<std::string>(), "Output parameters file");
	po::variables_map vm;
	try {
		po::parsed_options parsed = po::command_line_parser(argc,
		                                                    argv).options(desc).allow_unregistered().run();
		po::store(parsed, vm);
		po::notify(vm);
		if (task_type == "parse") {
			desc.add(parse_desc);
			po::store(po::parse_command_line(argc, argv, desc), vm);
			Params cmd_params = parse_cmd(vm);
			return cmd_params;
		} else {
			desc.add(parse_desc);
			std::cout << desc << std::endl;
		}
	} catch(std::exception& ex) {
		std::cout << desc << std::endl;
	}
	return Params();
}

int main(int argc, char* argv[]){
    try {
            Params cmd_params = command_line_processor(argc, argv);
            MyCrawler crawler(cmd_params);
            crawler.crawl_to_live();
    } catch(std::exception& ex) {
        std::cout << ex.what() << std::endl;
    }
    return 0;
}
