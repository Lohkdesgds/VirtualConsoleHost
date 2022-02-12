#include <Lunaris/Socket/socket.h>
#include <Lunaris/Process/process.h>

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <thread>
#include <mutex>
#include <memory>

#include "commu.h"

using namespace Lunaris;

const u_short my_port = 27015;

void run_client(const std::string&);
void run_server(const std::string&);

int main(int argc, char* argv[])
{
    std::string _tmpbuf;

    std::cout << "Mode (client/host)? ";
    std::cin >> _tmpbuf;

    if (_tmpbuf == "host") {
        std::cout << "Script (.sh file)? ";
        std::cin >> _tmpbuf;

        if (_tmpbuf.size() > 0) {
            run_server(_tmpbuf);
            return 0;
        }
        else {
            std::cout << "Invalid input." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
            return 1;
        }
    }
    else {
        std::cout << "IP Address? ";
        std::cin >> _tmpbuf;

        if (_tmpbuf.size() > 0) {
            run_client(_tmpbuf);
            return 0;
        }
        else {
            std::cout << "Invalid input." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
            return 1;
        }
    }
    return 0;
}

void run_client(const std::string& ip)
{
    std::cout << "Attempting to connect to '" << ip << "'..." << std::endl;

    TCP_client client;
    if (!client.setup(socket_config().set_ip_address(ip).set_port(my_port))) {
        std::cout << "Cannot connect to host." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));
        return;
    }
    std::cout << "Connected! Checking version..." << std::endl;

    each_pack fpack = generate_version();
    if (!client.send((char*)&fpack, sizeof(fpack))) {
        std::cout << "Cannot send to host. Got disconnected?" << std::endl;
        client.close_socket();
        std::this_thread::sleep_for(std::chrono::seconds(5));
        return;
    }

    each_pack bpack;
    if (!client.recv(bpack)) {
        std::cout << "Cannot receive from host. Got disconnected?" << std::endl;
        client.close_socket();
        std::this_thread::sleep_for(std::chrono::seconds(5));
        return;
    }

    if (!is_package_good(bpack)) {
        client.close_socket();
        std::cout << "Version mismatch or package corrupted. Client or host not up to date." << std::endl;
        std::cout << "Host protocol version: " << bpack.raw.version.current_version << std::endl;
        std::cout << "Your protocol version: " << comm_version << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));
        return;        
    }

    std::cout << "All good! Console is now an echo of that terminal." << std::endl;

    std::thread thr_autoping([&]{
        do {
            std::this_thread::sleep_for(std::chrono::seconds(10));

            auto pkg = generate_ping();
            client.send((char*)&pkg, sizeof(pkg));

        } while(client.has_socket());
    });

    std::string message_buf;

    while(client.has_socket())
    {
        each_pack pkg;
        if (!client.recv(pkg)) continue;

        if(!is_package_good(pkg)) {
            std::cout << "[WARN] One package was damaged. Skipped." << std::endl;
            continue;
        }

        switch(pkg.type) {
        case pack_type::LPACK_MESSAGE:
        {
            message_buf.append(pkg.raw.data, pkg.len);

            if (!pkg.has_more) { // end of message, print
                std::cout << "[CMD-MSG] " << message_buf << std::endl;
                message_buf.clear();
            }
        }
            break;
        case pack_type::LPACK_PING:
        {
            const unsigned long long delta = calculate_ms_delay(pkg);
            if (delta > 1500) std::cout << "[WARN] Ping high: " << (1e-3 * delta) << " s (expected lower than 1 second)" << std::endl;
        }
            break;
        }
    }

    std::cout << "Client got disconnected! Closing stuff..." << std::endl;

    if(thr_autoping.joinable()) thr_autoping.join();

    std::cout << "Quitting in 5 seconds." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
}

void run_server(const std::string& call)
{
    bool keep = true;

    TCP_host hq;

    std::vector<TCP_client> clients;
    std::mutex clients_mu;

    std::cout << "Setting up host..." << std::endl;

    if (!hq.setup(socket_config().set_port(my_port).set_family(socket_config::e_family::IPV6))) {
        std::cout << "Can't configure TCP host properly." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));
        return;
    }

    std::cout << "Working on auto accept clients..." << std::endl;

    std::thread thr_listen([&]{
        while(keep) {
            TCP_client cli = hq.listen(3);

            if (!cli.has_socket()) {
                std::lock_guard<std::mutex> luck(clients_mu);
                
                for (auto ite = clients.begin(); ite != clients.end();)
                {
                    if (!ite->has_socket()) {
                        std::cout << "[DEBUG] Disconnected: " << ite->info().ip_address << ". (" << clients.size() << ")" << std::endl;
                        ite = clients.erase(ite);
                    }
                    else ++ite;
                }

                continue;
            }

            each_pack fpack = generate_version();
            if (!cli.send((char*)&fpack, sizeof(fpack))) continue;

            each_pack bpack;
            if (!cli.recv(bpack)) continue;

            if (!is_package_good(bpack)) continue;

            std::lock_guard<std::mutex> luck(clients_mu);
            std::cout << "[DEBUG] New successful connection: " << cli.info().ip_address << ". (" << clients.size() << ")" << std::endl;
            clients.push_back(std::move(cli));
        }
    });

    std::cout << "Starting slow thread to answer ping..." << std::endl;

    std::thread thr_socketwork([&]{
        while(keep) {
            std::this_thread::sleep_for(std::chrono::seconds(1));

            std::lock_guard<std::mutex> luck(clients_mu);
            for(auto& it : clients) {

                auto fpkg = it.recv(sizeof(each_pack), false);
                if (fpkg.empty()) continue;

                each_pack* pkg = (each_pack*)fpkg.data();

                if (!is_package_good(*pkg)) continue;

                switch (pkg->type) {
                case pack_type::LPACK_PING:
                {
                    it.send((char*)pkg, sizeof(each_pack));
                }
                    break;
                default: // only supported feature right now is the ping. All stuff is read only this time.
                    break;
                }
            }
        }
    });

    std::cout << "Starting process..." << std::endl;

    process_sync sync("sh", {call}, process_sync::mode::READWRITE);

    std::thread thr_read([&]{
        do {
            std::string str = sync.read();
            std::cout << "[PROC] " << str << std::endl;

            auto packs = generate_of_message(str);

            std::lock_guard<std::mutex> luck(clients_mu);
            for(auto& each : clients) {
                for(const auto& it : packs) if (!each.send((char*)&it, sizeof(it))) break; // break on fail
            }

        } while(sync.has_read() || sync.is_running());
    });

    std::cout << "Listening to keyboard now. Everything is ready!" << std::endl;

    do {
        std::string in;
        std::getline(std::cin, in);

        sync.write(in);

        if (in == "stop")
            break;

    } while (sync.is_running());

    keep = false;

    if (thr_read.joinable()) thr_read.join();
    if (thr_listen.joinable()) thr_listen.join();
    if (thr_socketwork.joinable()) thr_socketwork.join();
}
