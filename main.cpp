#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <zmq.h>
#include <zmq.hpp>
#include <boost/json.hpp>

struct config_t {
    std::string pattern;
    std::string sub_addr;
    int sub_hwm = 0;
    int sub_timeout = 0;
    std::string pub_addr;
    int pub_hwm = 0;
    int pub_timeout = 0;
};

int load_config(config_t * dst, const char *fp);
void proxy_xsub_xpub(config_t *conf);

typedef void (*proxy_fn)(config_t*);
typedef std::map<std::string, proxy_fn> proxy_map;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "not enough arguments (path to config file)" << std::endl;
    }
    proxy_map patterns;
    patterns["XSUB_XPUB"] = proxy_xsub_xpub;

    config_t conf;
    auto ec = load_config(&conf, argv[1]);
    if (ec != EXIT_SUCCESS) { return ec; }

    std::cout << "starting " << conf.pattern << " broker" << std::endl;
    auto fn = patterns.find(conf.pattern);
    if (fn == patterns.end()) {
        std::cout << "unknown pattern" << std::endl;
        return EXIT_FAILURE;
    }
    fn->second(&conf);

    return EXIT_SUCCESS;
}

int load_config(config_t * dst, const char *fp) {
    try {
        std::ifstream f(fp);
        if (!f.is_open()) { return EXIT_FAILURE; }
        std::stringstream ss;
        ss << f.rdbuf();

        boost::json::stream_parser p;
        boost::json::error_code ec;

        p.write(ss.str().c_str(), ss.str().length(), ec);
        if (ec) { return EXIT_FAILURE; }
        p.finish(ec);
        if (ec) { return EXIT_FAILURE; }
        auto root = p.release().get_object();

        dst->pattern = root.at("pattern").as_string().c_str();

        auto sub = root.at("sub").get_object();
        dst->sub_addr = sub.at("addr").as_string().c_str();
        dst->sub_hwm = int(sub.at("hwm").as_int64());
        dst->sub_timeout = int(sub.at("timeo").as_int64());

        auto pub = root.at("pub").get_object();
        dst->pub_addr = pub.at("addr").as_string().c_str();
        dst->pub_hwm = int(pub.at("hwm").as_int64());
        dst->pub_timeout = int(pub.at("timeo").as_int64());
    } catch (int e) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void proxy_xsub_xpub(config_t *conf) {
    zmq::context_t ctx(1);

    zmq::socket_t ssub (ctx, ZMQ_XSUB);
    zmq_setsockopt(ssub, ZMQ_RCVHWM, &conf->sub_hwm, sizeof(conf->sub_hwm));
    zmq_setsockopt(ssub, ZMQ_RCVTIMEO, &conf->sub_timeout, sizeof(conf->sub_timeout));
    ssub.bind(conf->sub_addr);
    std::cout << " > XSUB socket address " << conf->sub_addr << std::endl;

    zmq::socket_t spub (ctx, ZMQ_XPUB);
    zmq_setsockopt(spub, ZMQ_SNDHWM, &conf->pub_hwm, sizeof(conf->pub_hwm));
    zmq_setsockopt(spub, ZMQ_SNDTIMEO, &conf->pub_timeout, sizeof(conf->pub_timeout));
    spub.bind(conf->pub_addr);
    std::cout << " > XPUB socket address " << conf->pub_addr << std::endl;

    zmq::proxy(ssub, spub);
}
