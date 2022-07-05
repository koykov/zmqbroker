#include <iostream>
#include <cstdlib>
#include <map>
#include <zmq.h>
#include <zmq.hpp>

struct config_t {
    std::string pattern;
    std::string sub_addr;
    int sub_hwm = 0;
    int sub_timeout = 0;
    std::string pub_addr;
    int pub_hwm = 0;
    int pub_timeout = 0;
};

const char *getenv_safe(const char *name);
int iany(const std::string& key, int def);
int load_config(config_t * dst);
void proxy_xsub_xpub(config_t *conf);

typedef void (*broker_fn)(config_t*);

static const std::map<std::string, broker_fn> PATTERNS = {
        {"XSUB_XPUB", proxy_xsub_xpub},
};

int main() {
    config_t conf;
    auto ec = load_config(&conf);
    if (ec != EXIT_SUCCESS) { return ec; }

    std::cout << "starting " << conf.pattern << " broker" << std::endl;
    auto fn = PATTERNS.find(conf.pattern);
    if (fn == PATTERNS.end()) {
        std::cout << "unknown pattern" << std::endl;
        return EXIT_FAILURE;
    }
    fn->second(&conf);

    return EXIT_SUCCESS;
}

int iany(const std::string& key, int def) {
    auto v = atoi(getenv_safe(key.c_str()));
    if (v <= 0) { return def; }
    return v;
}

const char *getenv_safe(const char *name) {
    auto p = std::getenv(name);
    if (p == nullptr) { return ""; }
    return p;
}

int load_config(config_t * dst) {
    dst->pattern = getenv_safe("PATTERN");
    if (dst->pattern.empty()) { return EXIT_FAILURE; }
    if (PATTERNS.find(dst->pattern) == PATTERNS.end()) { return EXIT_FAILURE; }

    dst->sub_addr = getenv_safe("SUB_ADDR");
    if (dst->sub_addr.empty()) { return EXIT_FAILURE; }
    dst->sub_hwm = iany("SUB_HWM", 1000000);
    dst->sub_timeout = iany("SUB_TIMEOUT", 100);

    dst->pub_addr = getenv_safe("PUB_ADDR");
    if (dst->pub_addr.empty()) { return EXIT_FAILURE; }
    dst->pub_hwm = iany("PUB_HWM", 1000000);
    dst->pub_timeout = iany("PUB_TIMEOUT", 100);

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
