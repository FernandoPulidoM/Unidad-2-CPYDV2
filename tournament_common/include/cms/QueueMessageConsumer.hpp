//
// Created by root on 9/24/25.
//

#ifndef COMMON_QUEUE_MESSAGE_CONSUMER_HPP
#define COMMON_QUEUE_MESSAGE_CONSUMER_HPP


#include <atomic>
#include <memory>
#include <thread>
#include <cms/MessageConsumer.h>
#include <cms/Session.h>
#include <functional>
#include <iostream>

#include "cms/ConnectionManager.hpp"

class QueueMessageConsumer : public cms::MessageListener {
    std::shared_ptr<ConnectionManager> connectionManager;
    std::atomic<bool> running;
    std::thread worker;
    // std::shared_ptr<cms::Connection> connection;
    std::shared_ptr<cms::Session> session;
    std::shared_ptr<cms::MessageConsumer> messageConsumer;
    // ← AGREGAR ESTO: callback para manejar mensajes
    std::function<void(const std::string&)> messageHandler;

    // void readMessage();
public:
    explicit QueueMessageConsumer(const std::shared_ptr<ConnectionManager>& connectionManager);
    ~QueueMessageConsumer();
    void Start(const std::string_view & queueName);
    void Stop();

    // ← AGREGAR ESTO: método para registrar el handler
    void SetMessageHandler(std::function<void(const std::string&)> handler) {
        messageHandler = std::move(handler);
    }

    virtual void onMessage(const cms::Message* message);
};

inline QueueMessageConsumer::QueueMessageConsumer(const std::shared_ptr<ConnectionManager>& connectionManager) : connectionManager(connectionManager) {
    std::cout << "Created QueueMessageConsumer";
}

inline QueueMessageConsumer::~QueueMessageConsumer() {
    Stop();
}

inline void QueueMessageConsumer::Start(const std::string_view& queueName) {
    if (this->running)
        return;
    this->running = true;
    try {
        session = connectionManager->CreateSession();
        const auto destination = std::unique_ptr<cms::Queue>(session->createQueue(queueName.data()));
        auto consumer = std::unique_ptr<cms::MessageConsumer>(session->createConsumer(destination.get()));

        while (running) {
            std::unique_ptr<cms::Message> message(consumer->receive(1500));
            if (message) {
                if (auto text = dynamic_cast<cms::TextMessage*>(message.get())) {
                    std::cout << "message consumed: {}" << text->getText() ;
                }
            }
        }
    } catch (const cms::CMSException& e) {

    }
}

inline void QueueMessageConsumer::Stop() {
    running = false;
    if (worker.joinable())
        worker.join();

    messageConsumer->close();
    session->close();
    // connection->close();
}

inline void QueueMessageConsumer::onMessage(const cms::Message* message) {
    try {
        if (auto textMsg = dynamic_cast<const cms::TextMessage*>(message)) {
            std::string text = textMsg->getText();
            std::cout << "Message received: {}\n" << text;

            // ← LLAMAR AL HANDLER SI EXISTE
            if (messageHandler) {
                messageHandler(text);
            }
        }
    } catch (const cms::CMSException& e) {
        std::cout << "Error processing message: {}\n" << e.what();
    }
}

// inline void QueueMessageConsumer::readMessage() {
//     try {
//         if (this->running)
//             return;
//         session = connectionManager->CreateSession();
//         const auto destination = std::unique_ptr<cms::Destination>(session->createQueue(queueName));
//         auto consumer = std::unique_ptr<cms::MessageConsumer>(session->createConsumer(destination.get()));
//
//         while (running) {
//             std::unique_ptr<cms::Message> message(consumer->receive(1000));
//             if (message) {
//                 if (auto text = dynamic_cast<cms::TextMessage*>(message.get())) {
//                     std::cout << "message consumed: {}" << text->getText();
//                 }
//             }
//         }
//     } catch (const cms::CMSException& e) {
//
//     }
// }

#endif //COMMON_QUEUE_MESSAGE_CONSUMER_HPP