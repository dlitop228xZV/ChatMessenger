#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <crow.h>
#include <sqlite3.h>

using namespace std;

// Структуры данных
struct User {
    int id;
    string name;
    string login;
    string password;
};

// Структура для информации о пользователе
struct UserInfo {
    int id;
    string name;
    string login;
};

// Структура для поиска пользователей
struct UserSearchResult {
    int id;
    string name;
    string login;
};

struct Chat {
    int id;
    string name;
    bool isGroup;
    int createdBy;
    string createdAt;
};

struct Message {
    int id;
    int userId;
    int chatId;
    string msg;
    int replyId; // 0 если нет reply
    string sendDate;
    int resendId; // 0 если нет пересылки
};

struct Contact {
    int id;
    int userId1;
    int userId2;
};

class Database {
private:
    sqlite3* db;

    // Хелпер функция для выполнения SQL запросов
    bool executeSQL(const string& sql,
        const vector<pair<int, string>>& params = {},
        function<void(sqlite3_stmt*)> callback = nullptr) {
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            cerr << "SQL error: " << sqlite3_errmsg(db) << endl;
            return false;
        }

        // Биндим параметры
        for (size_t i = 0; i < params.size(); i++) {
            int index = i + 1;
            int type = params[i].first;
            const string& value = params[i].second;

            if (type == SQLITE_INTEGER) {
                sqlite3_bind_int(stmt, index, stoi(value));
            }
            else if (type == SQLITE_TEXT) {
                sqlite3_bind_text(stmt, index, value.c_str(), -1, SQLITE_TRANSIENT);
            }
        }

        bool result = true;
        if (callback) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                callback(stmt);
            }
        }
        else {
            result = sqlite3_step(stmt) == SQLITE_DONE;
        }

        sqlite3_finalize(stmt);
        return result;
    }

public:
    Database(const string& dbPath = "chat.db") {
        if (sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK) {
            throw runtime_error("Can't open database: " + string(sqlite3_errmsg(db)));
        }
        createTables();
    }

    ~Database() {
        if (db) {
            sqlite3_close(db);
        }
    }

    void createTables() {
        const char* tables[] = {
            R"(
                CREATE TABLE IF NOT EXISTS users (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    name TEXT NOT NULL,
                    login TEXT UNIQUE NOT NULL,
                    password TEXT NOT NULL
                )
            )",
            R"(
                CREATE TABLE IF NOT EXISTS chats (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    name TEXT,
                    is_group INTEGER DEFAULT 0,
                    created_by INTEGER,
                    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
                )
            )",
            R"(
                CREATE TABLE IF NOT EXISTS messages (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    user_id INTEGER NOT NULL,
                    chat_id INTEGER NOT NULL,
                    msg TEXT NOT NULL,
                    reply_id INTEGER DEFAULT 0,
                    send_date DATETIME DEFAULT CURRENT_TIMESTAMP,
                    resend_id INTEGER DEFAULT 0
                )
            )",
            R"(
                CREATE TABLE IF NOT EXISTS contacts (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    user_id1 INTEGER NOT NULL,
                    user_id2 INTEGER NOT NULL,
                    CHECK (user_id1 != user_id2)
                )
            )",
            R"(
                CREATE TABLE IF NOT EXISTS user_chats (
                    user_id INTEGER NOT NULL,
                    chat_id INTEGER NOT NULL,
                    PRIMARY KEY (user_id, chat_id)
                )
            )"
        };

        for (const char* table : tables) {
            char* errMsg = nullptr;
            if (sqlite3_exec(db, table, nullptr, nullptr, &errMsg) != SQLITE_OK) {
                string error = "SQL error: " + string(errMsg);
                sqlite3_free(errMsg);
                cerr << error << endl;
            }
        }
    }

    // Регистрация пользователя
    int registerUser(const string& name, const string& login, const string& password) {
        string hashedPassword = to_string(hash<string>{}(password));
        string sql = "INSERT INTO users (name, login, password) VALUES (?, ?, ?)";

        vector<pair<int, string>> params = {
            {SQLITE_TEXT, name},
            {SQLITE_TEXT, login},
            {SQLITE_TEXT, hashedPassword}
        };

        if (!executeSQL(sql, params)) {
            return -1;
        }

        return sqlite3_last_insert_rowid(db);
    }

    // Авторизация пользователя
    bool loginUser(const string& login, const string& password, User& user) {
        string sql = "SELECT id, name, login, password FROM users WHERE login = ?";
        vector<pair<int, string>> params = { {SQLITE_TEXT, login} };

        bool found = false;
        auto callback = [&](sqlite3_stmt* stmt) {
            string storedHash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            string inputHash = to_string(hash<string>{}(password));

            if (inputHash == storedHash) {
                user.id = sqlite3_column_int(stmt, 0);
                user.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
                user.login = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
                found = true;
            }
            };

        executeSQL(sql, params, callback);
        return found;
    }

    // Получение пользователя по ID
    bool getUserById(int userId, UserInfo& user) {
        string sql = "SELECT id, name, login FROM users WHERE id = ?";
        vector<pair<int, string>> params = { {SQLITE_INTEGER, to_string(userId)} };

        bool found = false;
        auto callback = [&](sqlite3_stmt* stmt) {
            user.id = sqlite3_column_int(stmt, 0);
            user.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            user.login = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            found = true;
            };

        executeSQL(sql, params, callback);
        return found;
    }

    // Поиск пользователей
    vector<UserSearchResult> searchUsers(const string& searchQuery) {
        vector<UserSearchResult> result;

        string sql = "SELECT id, name, login FROM users WHERE (login LIKE ? OR name LIKE ?) AND id > 0";
        vector<pair<int, string>> params = {
            {SQLITE_TEXT, "%" + searchQuery + "%"},
            {SQLITE_TEXT, "%" + searchQuery + "%"}
        };

        auto callback = [&](sqlite3_stmt* stmt) {
            UserSearchResult user;
            user.id = sqlite3_column_int(stmt, 0);
            user.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            user.login = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            result.push_back(user);
            };

        executeSQL(sql, params, callback);
        return result;
    }

    // Создание чата
    int createChat(const string& name, bool isGroup, int createdBy, const vector<int>& participants) {
        string sql = "INSERT INTO chats (name, is_group, created_by) VALUES (?, ?, ?)";

        vector<pair<int, string>> params = {
            {SQLITE_TEXT, name},
            {SQLITE_INTEGER, isGroup ? "1" : "0"},
            {SQLITE_INTEGER, to_string(createdBy)}
        };

        if (!executeSQL(sql, params)) {
            return -1;
        }

        int chatId = sqlite3_last_insert_rowid(db);

        // Добавляем всех участников
        for (int userId : participants) {
            addUserToChat(userId, chatId);
        }

        // Добавляем создателя
        addUserToChat(createdBy, chatId);

        return chatId;
    }

    // Добавление пользователя в чат
    bool addUserToChat(int userId, int chatId) {
        string sql = "INSERT OR IGNORE INTO user_chats (user_id, chat_id) VALUES (?, ?)";

        vector<pair<int, string>> params = {
            {SQLITE_INTEGER, to_string(userId)},
            {SQLITE_INTEGER, to_string(chatId)}
        };

        return executeSQL(sql, params);
    }

    // Добавление контакта
    int addContact(int userId1, int userId2) {
        if (userId1 == userId2) {
            return -1; // Нельзя добавить самого себя
        }

        // Проверяем существование пользователей
        UserInfo user1, user2;
        bool user1Exists = getUserById(userId1, user1);
        bool user2Exists = getUserById(userId2, user2);

        if (!user1Exists || !user2Exists) {
            return -3; // Один из пользователей не найден
        }

        // Проверяем, существует ли уже контакт
        string checkSql = R"(
            SELECT id FROM contacts 
            WHERE (user_id1 = ? AND user_id2 = ?) OR (user_id1 = ? AND user_id2 = ?)
        )";

        vector<pair<int, string>> checkParams = {
            {SQLITE_INTEGER, to_string(userId1)},
            {SQLITE_INTEGER, to_string(userId2)},
            {SQLITE_INTEGER, to_string(userId2)},
            {SQLITE_INTEGER, to_string(userId1)}
        };

        bool exists = false;
        auto checkCallback = [&](sqlite3_stmt* stmt) {
            exists = true;
            };

        executeSQL(checkSql, checkParams, checkCallback);

        if (exists) {
            return -2; // Контакт уже существует
        }

        string sql = "INSERT INTO contacts (user_id1, user_id2) VALUES (?, ?)";
        vector<pair<int, string>> params = {
            {SQLITE_INTEGER, to_string(userId1)},
            {SQLITE_INTEGER, to_string(userId2)}
        };

        if (!executeSQL(sql, params)) {
            return -4; // Ошибка базы данных
        }

        return sqlite3_last_insert_rowid(db);
    }

    // Получение чатов пользователя
    vector<Chat> getUserChats(int userId) {
        vector<Chat> result;

        string sql = R"(
            SELECT c.id, c.name, c.is_group, c.created_by, c.created_at
            FROM chats c
            JOIN user_chats uc ON c.id = uc.chat_id
            WHERE uc.user_id = ?
            ORDER BY c.created_at DESC
        )";

        vector<pair<int, string>> params = { {SQLITE_INTEGER, to_string(userId)} };

        auto callback = [&](sqlite3_stmt* stmt) {
            Chat chat;
            chat.id = sqlite3_column_int(stmt, 0);
            chat.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            chat.isGroup = sqlite3_column_int(stmt, 2) == 1;
            chat.createdBy = sqlite3_column_int(stmt, 3);
            chat.createdAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            result.push_back(chat);
            };

        executeSQL(sql, params, callback);
        return result;
    }

    // Получение контактов пользователя
    vector<pair<int, string>> getUserContacts(int userId) {
        vector<pair<int, string>> result;

        string sql = R"(
            SELECT 
                CASE 
                    WHEN c.user_id1 = ? THEN c.user_id2
                    ELSE c.user_id1
                END as other_user_id,
                u.name
            FROM contacts c
            JOIN users u ON (c.user_id1 = u.id OR c.user_id2 = u.id) AND u.id != ?
            WHERE (c.user_id1 = ? OR c.user_id2 = ?)
        )";

        vector<pair<int, string>> params = {
            {SQLITE_INTEGER, to_string(userId)},
            {SQLITE_INTEGER, to_string(userId)},
            {SQLITE_INTEGER, to_string(userId)},
            {SQLITE_INTEGER, to_string(userId)}
        };

        auto callback = [&](sqlite3_stmt* stmt) {
            int otherUserId = sqlite3_column_int(stmt, 0);
            string name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            result.push_back({ otherUserId, name });
            };

        executeSQL(sql, params, callback);
        return result;
    }

    // Отправка сообщения
    int sendMessage(int userId, int chatId, const string& message, int replyId = 0, int resendId = 0) {
        string sql = "INSERT INTO messages (user_id, chat_id, msg, reply_id, resend_id) VALUES (?, ?, ?, ?, ?)";

        vector<pair<int, string>> params = {
            {SQLITE_INTEGER, to_string(userId)},
            {SQLITE_INTEGER, to_string(chatId)},
            {SQLITE_TEXT, message},
            {SQLITE_INTEGER, to_string(replyId)},
            {SQLITE_INTEGER, to_string(resendId)}
        };

        if (!executeSQL(sql, params)) {
            return -1;
        }

        return sqlite3_last_insert_rowid(db);
    }

    // Получение сообщений чата
    vector<Message> getChatMessages(int chatId) {
        vector<Message> result;

        string sql = R"(
            SELECT id, user_id, msg, reply_id, send_date, resend_id
            FROM messages
            WHERE chat_id = ?
            ORDER BY send_date ASC
        )";

        vector<pair<int, string>> params = { {SQLITE_INTEGER, to_string(chatId)} };

        auto callback = [&](sqlite3_stmt* stmt) {
            Message msg;
            msg.id = sqlite3_column_int(stmt, 0);
            msg.userId = sqlite3_column_int(stmt, 1);
            msg.msg = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            msg.replyId = sqlite3_column_int(stmt, 3);
            msg.sendDate = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            msg.resendId = sqlite3_column_int(stmt, 5);
            msg.chatId = chatId;
            result.push_back(msg);
            };

        executeSQL(sql, params, callback);
        return result;
    }

    // Редактирование сообщения
    bool editMessage(int messageId, const string& newMessage, int userId) {
        string sql = "UPDATE messages SET msg = ? WHERE id = ? AND user_id = ?";

        vector<pair<int, string>> params = {
            {SQLITE_TEXT, newMessage},
            {SQLITE_INTEGER, to_string(messageId)},
            {SQLITE_INTEGER, to_string(userId)}
        };

        return executeSQL(sql, params);
    }

    // Удаление сообщения
    bool deleteMessage(int messageId, int userId) {
        string sql = "DELETE FROM messages WHERE id = ? AND user_id = ?";

        vector<pair<int, string>> params = {
            {SQLITE_INTEGER, to_string(messageId)},
            {SQLITE_INTEGER, to_string(userId)}
        };

        return executeSQL(sql, params);
    }

    // Получение информации о сообщении
    bool getMessageInfo(int messageId, int& userId, string& msg) {
        string sql = "SELECT user_id, msg FROM messages WHERE id = ?";
        vector<pair<int, string>> params = { {SQLITE_INTEGER, to_string(messageId)} };

        bool found = false;
        auto callback = [&](sqlite3_stmt* stmt) {
            userId = sqlite3_column_int(stmt, 0);
            msg = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            found = true;
            };

        executeSQL(sql, params, callback);
        return found;
    }
};

class ChatServer {
private:
    crow::SimpleApp app;
    unique_ptr<Database> db;

public:
    ChatServer() : db(make_unique<Database>()) {
        setupRoutes();
    }

    void run(int port = 18080) {
        cout << "Chat Server running on port " << port << endl;
        app.port(port).multithreaded().run();
    }

private:
    void setupRoutes() {
        // Регистрация
        CROW_ROUTE(app, "/auth/register").methods("POST"_method)
            ([this](const crow::request& req) {
            try {
                auto json_body = crow::json::load(req.body);
                if (!json_body) {
                    return crow::response(400, "Invalid JSON");
                }

                string name = json_body["name"].s();
                string login = json_body["login"].s();
                string password = json_body["password"].s();

                int userId = db->registerUser(name, login, password);
                if (userId == -1) {
                    crow::json::wvalue error;
                    error["error"] = "Registration failed (user may already exist)";
                    return crow::response(400, error);
                }

                crow::json::wvalue response;
                response["id"] = userId;
                response["status"] = "success";
                return crow::response(200, response);
            }
            catch (const exception& e) {
                crow::json::wvalue error;
                error["error"] = string("Error: ") + e.what();
                return crow::response(500, error);
            }
                });

        // Авторизация
        CROW_ROUTE(app, "/auth/login").methods("POST"_method)
            ([this](const crow::request& req) {
            try {
                auto json_body = crow::json::load(req.body);
                if (!json_body) {
                    return crow::response(400, "Invalid JSON");
                }

                string login = json_body["login"].s();
                string password = json_body["password"].s();

                User user;
                if (db->loginUser(login, password, user)) {
                    crow::json::wvalue response;
                    response["id"] = user.id;
                    response["name"] = user.name;
                    response["login"] = user.login;
                    response["status"] = "success";
                    return crow::response(200, response);
                }

                crow::json::wvalue error;
                error["error"] = "Invalid credentials";
                return crow::response(401, error);
            }
            catch (const exception& e) {
                crow::json::wvalue error;
                error["error"] = string("Error: ") + e.what();
                return crow::response(500, error);
            }
                });

        // Получение чатов пользователя
        CROW_ROUTE(app, "/chats/<int>").methods("GET"_method)
            ([this](int userId) {
            try {
                auto chats = db->getUserChats(userId);

                crow::json::wvalue response;
                response["status"] = "success";

                crow::json::wvalue::list chatList;
                for (const auto& chat : chats) {
                    crow::json::wvalue chatJson;
                    chatJson["id"] = chat.id;
                    chatJson["name"] = chat.name;
                    chatJson["isGroup"] = chat.isGroup;
                    chatJson["createdBy"] = chat.createdBy;
                    chatJson["createdAt"] = chat.createdAt;
                    chatList.push_back(chatJson);
                }
                response["chats"] = move(chatList);

                return crow::response(200, response);
            }
            catch (const exception& e) {
                crow::json::wvalue error;
                error["error"] = string("Error: ") + e.what();
                return crow::response(500, error);
            }
                });

        // Создание чата
        CROW_ROUTE(app, "/chats").methods("POST"_method)
            ([this](const crow::request& req) {
            try {
                auto json_body = crow::json::load(req.body);
                if (!json_body) {
                    return crow::response(400, "Invalid JSON");
                }

                string name = json_body["name"].s();
                bool isGroup = json_body["isGroup"].b();
                int createdBy = static_cast<int>(json_body["createdBy"].i());

                vector<int> participants;
                if (json_body.has("participants")) {
                    auto participants_array = json_body["participants"];
                    if (participants_array.t() == crow::json::type::List) {
                        for (size_t i = 0; i < participants_array.size(); i++) {
                            participants.push_back(static_cast<int>(participants_array[i].i()));
                        }
                    }
                }

                int chatId = db->createChat(name, isGroup, createdBy, participants);
                if (chatId == -1) {
                    crow::json::wvalue error;
                    error["error"] = "Failed to create chat";
                    return crow::response(400, error);
                }

                crow::json::wvalue response;
                response["id"] = chatId;
                response["status"] = "success";
                return crow::response(200, response);
            }
            catch (const exception& e) {
                crow::json::wvalue error;
                error["error"] = string("Error: ") + e.what();
                return crow::response(500, error);
            }
                });

        // Добавление контакта
        CROW_ROUTE(app, "/contacts").methods("POST"_method)
            ([this](const crow::request& req) {
            try {
                auto json_body = crow::json::load(req.body);
                if (!json_body) {
                    crow::json::wvalue error;
                    error["error"] = "Invalid JSON";
                    return crow::response(400, error);
                }

                int userId1 = static_cast<int>(json_body["userId1"].i());
                int userId2 = static_cast<int>(json_body["userId2"].i());

                int result = db->addContact(userId1, userId2);

                crow::json::wvalue response;
                if (result == -1) {
                    response["error"] = "Cannot add yourself as contact";
                    return crow::response(400, response);
                }
                else if (result == -2) {
                    response["error"] = "Contact already exists";
                    return crow::response(400, response);
                }
                else if (result == -3) {
                    response["error"] = "User not found";
                    return crow::response(404, response);
                }
                else if (result == -4) {
                    response["error"] = "Database error";
                    return crow::response(500, response);
                }
                else if (result > 0) {
                    response["id"] = result;
                    response["status"] = "success";
                    return crow::response(200, response);
                }
                else {
                    response["error"] = "Unknown error";
                    return crow::response(500, response);
                }
            }
            catch (const exception& e) {
                crow::json::wvalue error;
                error["error"] = string("Error: ") + e.what();
                return crow::response(500, error);
            }
                });

        // Получение контактов
        CROW_ROUTE(app, "/contacts/<int>").methods("GET"_method)
            ([this](int userId) {
            try {
                auto contacts = db->getUserContacts(userId);

                crow::json::wvalue response;
                response["status"] = "success";

                crow::json::wvalue::list contactList;
                for (const auto& contact : contacts) {
                    crow::json::wvalue contactJson;
                    contactJson["userId"] = contact.first;
                    contactJson["name"] = contact.second;
                    contactList.push_back(contactJson);
                }
                response["contacts"] = move(contactList);

                return crow::response(200, response);
            }
            catch (const exception& e) {
                crow::json::wvalue error;
                error["error"] = string("Error: ") + e.what();
                return crow::response(500, error);
            }
                });

        // Поиск пользователей
        CROW_ROUTE(app, "/users/search/<string>").methods("GET"_method)
            ([this](const string& searchQuery) {
            try {
                auto users = db->searchUsers(searchQuery);

                crow::json::wvalue response;
                response["status"] = "success";

                crow::json::wvalue::list usersList;
                for (const auto& user : users) {
                    crow::json::wvalue userJson;
                    userJson["id"] = user.id;
                    userJson["name"] = user.name;
                    userJson["login"] = user.login;
                    usersList.push_back(userJson);
                }
                response["users"] = move(usersList);

                return crow::response(200, response);
            }
            catch (const exception& e) {
                crow::json::wvalue error;
                error["error"] = string("Error: ") + e.what();
                return crow::response(500, error);
            }
                });

        // Получение пользователя по ID
        CROW_ROUTE(app, "/users/<int>").methods("GET"_method)
            ([this](int userId) {
            try {
                UserInfo user;
                bool found = db->getUserById(userId, user);

                if (found) {
                    crow::json::wvalue response;
                    response["id"] = user.id;
                    response["name"] = user.name;
                    response["login"] = user.login;
                    response["status"] = "success";
                    return crow::response(200, response);
                }
                else {
                    crow::json::wvalue error;
                    error["error"] = "User not found";
                    return crow::response(404, error);
                }
            }
            catch (const exception& e) {
                crow::json::wvalue error;
                error["error"] = string("Error: ") + e.what();
                return crow::response(500, error);
            }
                });

        // Получение сообщений чата
        CROW_ROUTE(app, "/chats/<int>/messages").methods("GET"_method)
            ([this](int chatId) {
            try {
                auto messages = db->getChatMessages(chatId);

                crow::json::wvalue response;
                response["status"] = "success";

                crow::json::wvalue::list messageList;
                for (const auto& msg : messages) {
                    crow::json::wvalue msgJson;
                    msgJson["id"] = msg.id;
                    msgJson["userId"] = msg.userId;
                    msgJson["message"] = msg.msg;
                    msgJson["replyId"] = msg.replyId;
                    msgJson["sendDate"] = msg.sendDate;
                    msgJson["resendId"] = msg.resendId;
                    messageList.push_back(msgJson);
                }
                response["messages"] = move(messageList);

                return crow::response(200, response);
            }
            catch (const exception& e) {
                crow::json::wvalue error;
                error["error"] = string("Error: ") + e.what();
                return crow::response(500, error);
            }
                });

        // Отправка сообщения
        CROW_ROUTE(app, "/messages").methods("POST"_method)
            ([this](const crow::request& req) {
            try {
                auto json_body = crow::json::load(req.body);
                if (!json_body) {
                    return crow::response(400, "Invalid JSON");
                }

                int userId = static_cast<int>(json_body["userId"].i());
                int chatId = static_cast<int>(json_body["chatId"].i());
                string message = json_body["message"].s();

                int replyId = 0;
                int resendId = 0;

                if (json_body.has("replyId")) {
                    replyId = static_cast<int>(json_body["replyId"].i());
                }

                if (json_body.has("resendId")) {
                    resendId = static_cast<int>(json_body["resendId"].i());
                }

                int messageId = db->sendMessage(userId, chatId, message, replyId, resendId);
                if (messageId == -1) {
                    crow::json::wvalue error;
                    error["error"] = "Failed to send message";
                    return crow::response(400, error);
                }

                crow::json::wvalue response;
                response["id"] = messageId;
                response["status"] = "success";
                return crow::response(200, response);
            }
            catch (const exception& e) {
                crow::json::wvalue error;
                error["error"] = string("Error: ") + e.what();
                return crow::response(500, error);
            }
                });

        // Редактирование сообщения
        CROW_ROUTE(app, "/messages/<int>").methods("PUT"_method)
            ([this](const crow::request& req, int messageId) {
            try {
                auto json_body = crow::json::load(req.body);
                if (!json_body) {
                    return crow::response(400, "Invalid JSON");
                }

                string newMessage = json_body["message"].s();
                int userId = static_cast<int>(json_body["userId"].i());

                bool success = db->editMessage(messageId, newMessage, userId);

                crow::json::wvalue response;
                if (success) {
                    response["status"] = "success";
                    return crow::response(200, response);
                }
                else {
                    response["error"] = "Message not found or access denied";
                    return crow::response(403, response);
                }
            }
            catch (const exception& e) {
                crow::json::wvalue error;
                error["error"] = string("Error: ") + e.what();
                return crow::response(500, error);
            }
                });

        // Удаление сообщения
        CROW_ROUTE(app, "/messages/<int>").methods("DELETE"_method)
            ([this](const crow::request& req, int messageId) {
            try {
                auto json_body = crow::json::load(req.body);
                if (!json_body) {
                    return crow::response(400, "Invalid JSON");
                }

                int userId = static_cast<int>(json_body["userId"].i());

                bool success = db->deleteMessage(messageId, userId);

                crow::json::wvalue response;
                if (success) {
                    response["status"] = "success";
                    return crow::response(200, response);
                }
                else {
                    response["error"] = "Message not found or access denied";
                    return crow::response(403, response);
                }
            }
            catch (const exception& e) {
                crow::json::wvalue error;
                error["error"] = string("Error: ") + e.what();
                return crow::response(500, error);
            }
                });

        // Пересылка сообщения
        CROW_ROUTE(app, "/messages/forward").methods("POST"_method)
            ([this](const crow::request& req) {
            try {
                auto json_body = crow::json::load(req.body);
                if (!json_body) {
                    return crow::response(400, "Invalid JSON");
                }

                int originalMsgId = static_cast<int>(json_body["originalMessageId"].i());
                int targetChatId = static_cast<int>(json_body["targetChatId"].i());
                int userId = static_cast<int>(json_body["userId"].i());

                // Получаем информацию о пересылаемом сообщении
                int originalUserId;
                string originalMsg;
                if (!db->getMessageInfo(originalMsgId, originalUserId, originalMsg)) {
                    crow::json::wvalue error;
                    error["error"] = "Original message not found";
                    return crow::response(404, error);
                }

                // Отправляем пересланное сообщение
                string forwardedMsg = "[Forwarded] " + originalMsg;
                int messageId = db->sendMessage(userId, targetChatId, forwardedMsg, 0, originalUserId);

                crow::json::wvalue response;
                response["id"] = messageId;
                response["status"] = "success";
                return crow::response(200, response);
            }
            catch (const exception& e) {
                crow::json::wvalue error;
                error["error"] = string("Error: ") + e.what();
                return crow::response(500, error);
            }
                });

        // Проверка работы сервера
        CROW_ROUTE(app, "/")([]() {
            return "Chat Messenger Server is running!";
            });
    }
};

int main() {
    try {
        ChatServer server;
        server.run(18080);
    }
    catch (const exception& e) {
        cerr << "Server error: " << e.what() << endl;
        return 1;
    }
    return 0;
}