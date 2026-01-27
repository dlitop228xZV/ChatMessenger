using System;
using System.Net.Http;
using System.Text;
using System.Text.Json;
using System.Threading.Tasks;

namespace ChatClient
{
    class Program
    {
        private static readonly HttpClient client = new HttpClient();
        private static readonly string baseUrl = "http://localhost:18080";
        private static int currentUserId = 0;

        static async Task Main(string[] args)
        {
            Console.Clear();

            Console.WriteLine("=== Chat Messenger ===");

            while (true)
            {
                Console.WriteLine("\n1. Регистрация");
                Console.WriteLine("2. Вход");
                Console.WriteLine("3. Мои чаты");
                Console.WriteLine("4. Создать чат");
                Console.WriteLine("5. Добавить контакт");
                Console.WriteLine("6. Отправить сообщение");
                Console.WriteLine("7. Выход");
                Console.Write("Выберите действие: ");

                var choice = Console.ReadLine();

                switch (choice)
                {
                    case "1":
                        await Register();
                        break;
                    case "2":
                        await Login();
                        break;
                    case "3":
                        await GetMyChats();
                        break;
                    case "4":
                        await CreateChat();
                        break;
                    case "5":
                        await AddContact();
                        break;
                    case "6":
                        await SendMessage();
                        break;
                    case "7":
                        return;
                }
            }
        }

        static async Task Register()
        {
            Console.Write("Имя: ");
            string name = Console.ReadLine();
            Console.Write("Логин: ");
            string login = Console.ReadLine();
            Console.Write("Пароль: ");
            string password = Console.ReadLine();

            var user = new { name, login, password };
            var json = JsonSerializer.Serialize(user);
            var content = new StringContent(json, Encoding.UTF8, "application/json");

            var response = await client.PostAsync($"{baseUrl}/auth/register", content);
            var responseString = await response.Content.ReadAsStringAsync();

            Console.WriteLine($"Ответ: {responseString}");
        }

        static async Task Login()
        {
            Console.Write("Логин: ");
            string login = Console.ReadLine();
            Console.Write("Пароль: ");
            string password = Console.ReadLine();

            var credentials = new { login, password };
            var json = JsonSerializer.Serialize(credentials);
            var content = new StringContent(json, Encoding.UTF8, "application/json");

            var response = await client.PostAsync($"{baseUrl}/auth/login", content);
            var responseString = await response.Content.ReadAsStringAsync();

            if (response.IsSuccessStatusCode)
            {
                var result = JsonSerializer.Deserialize<JsonElement>(responseString);
                currentUserId = result.GetProperty("id").GetInt32();
                Console.WriteLine($"Вход выполнен! ID пользователя: {currentUserId}");
            }
            else
            {
                Console.WriteLine($"Ошибка: {responseString}");
            }
        }

        static async Task GetMyChats()
        {
            if (currentUserId == 0)
            {
                Console.WriteLine("Сначала войдите в систему!");
                return;
            }

            var response = await client.GetAsync($"{baseUrl}/chats/{currentUserId}");
            var responseString = await response.Content.ReadAsStringAsync();

            Console.WriteLine("Ваши чаты:");
            Console.WriteLine(responseString);
        }

        static async Task CreateChat()
        {
            if (currentUserId == 0)
            {
                Console.WriteLine("Сначала войдите в систему!");
                return;
            }

            Console.Write("Название чата: ");
            string name = Console.ReadLine();
            Console.Write("Групповой чат? (y/n): ");
            bool isGroup = Console.ReadLine().ToLower() == "y";

            Console.Write("Участники (через запятую, ID): ");
            string participantsInput = Console.ReadLine();
            var participants = participantsInput.Split(',');

            var chatData = new
            {
                name,
                isGroup,
                createdBy = currentUserId,
                participants = Array.ConvertAll(participants, int.Parse)
            };

            var json = JsonSerializer.Serialize(chatData);
            var content = new StringContent(json, Encoding.UTF8, "application/json");

            var response = await client.PostAsync($"{baseUrl}/chats", content);
            var responseString = await response.Content.ReadAsStringAsync();

            Console.WriteLine($"Ответ: {responseString}");
        }

        static async Task AddContact()
        {
            if (currentUserId == 0)
            {
                Console.WriteLine("Сначала войдите в систему!");
                return;
            }

            Console.Write("ID пользователя для добавления: ");
            if (!int.TryParse(Console.ReadLine(), out int friendId))
            {
                Console.WriteLine("Неверный ID!");
                return;
            }

            var contactData = new { userId1 = currentUserId, userId2 = friendId };
            var json = JsonSerializer.Serialize(contactData);
            var content = new StringContent(json, Encoding.UTF8, "application/json");

            var response = await client.PostAsync($"{baseUrl}/contacts", content);
            var responseString = await response.Content.ReadAsStringAsync();

            Console.WriteLine($"Ответ: {responseString}");
        }

        static async Task SendMessage()
        {
            if (currentUserId == 0)
            {
                Console.WriteLine("Сначала войдите в систему!");
                return;
            }

            Console.Write("ID чата: ");
            if (!int.TryParse(Console.ReadLine(), out int chatId))
            {
                Console.WriteLine("Неверный ID чата!");
                return;
            }

            Console.Write("Сообщение: ");
            string message = Console.ReadLine();

            var messageData = new
            {
                userId = currentUserId,
                chatId,
                message
            };

            var json = JsonSerializer.Serialize(messageData);
            var content = new StringContent(json, Encoding.UTF8, "application/json");

            var response = await client.PostAsync($"{baseUrl}/messages", content);
            var responseString = await response.Content.ReadAsStringAsync();

            Console.WriteLine($"Ответ: {responseString}");
        }
    }
}