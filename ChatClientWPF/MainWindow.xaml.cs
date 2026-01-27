using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.Http;
using System.Text;
using System.Text.Json;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;

namespace ChatClientWPF
{
    public partial class MainWindow : Window
    {
        private HttpClient client = new HttpClient();
        private string baseUrl = "http://localhost:18080";
        private int currentUserId = 0;
        private string currentUserName = "";
        private int currentChatId = 0;

        public class User
        {
            public int Id { get; set; }
            public string Name { get; set; }
            public string Login { get; set; }
        }

        public class Chat
        {
            public int Id { get; set; }
            public string Name { get; set; }
            public bool IsGroup { get; set; }
            public int CreatedBy { get; set; }
            public string CreatedAt { get; set; }
        }

        public class Message
        {
            public int Id { get; set; }
            public int UserId { get; set; }
            public string UserName { get; set; }
            public string Content { get; set; }
            public string Timestamp { get; set; }
        }

        public class Contact
        {
            public int UserId { get; set; }
            public string Name { get; set; }
            public string Login { get; set; }
        }

        public MainWindow()
        {
            InitializeComponent();
            tbUserInfo.Text = "Не авторизован";
            client.Timeout = TimeSpan.FromSeconds(30);
        }

        // Регистрация
        private async void BtnRegister_Click(object sender, RoutedEventArgs e)
        {
            var dialog = new RegisterDialog();
            if (dialog.ShowDialog() == true)
            {
                try
                {
                    var userData = new
                    {
                        name = dialog.UserName,
                        login = dialog.UserLogin,
                        password = dialog.Password
                    };

                    var json = JsonSerializer.Serialize(userData);
                    var content = new StringContent(json, Encoding.UTF8, "application/json");

                    var response = await client.PostAsync($"{baseUrl}/auth/register", content);
                    var responseString = await response.Content.ReadAsStringAsync();

                    if (response.IsSuccessStatusCode)
                    {
                        var result = JsonSerializer.Deserialize<JsonElement>(responseString);
                        MessageBox.Show($"Регистрация успешна! ID: {result.GetProperty("id").GetInt32()}",
                            "Успех", MessageBoxButton.OK, MessageBoxImage.Information);
                    }
                    else
                    {
                        MessageBox.Show($"Ошибка: {responseString}", "Ошибка",
                            MessageBoxButton.OK, MessageBoxImage.Error);
                    }
                }
                catch (Exception ex)
                {
                    MessageBox.Show($"Ошибка соединения: {ex.Message}", "Ошибка",
                        MessageBoxButton.OK, MessageBoxImage.Error);
                }
            }
        }

        // Вход
        private async void BtnLogin_Click(object sender, RoutedEventArgs e)
        {
            var dialog = new LoginDialog();
            if (dialog.ShowDialog() == true)
            {
                try
                {
                    var credentials = new
                    {
                        login = dialog.UserLogin,
                        password = dialog.Password
                    };

                    var json = JsonSerializer.Serialize(credentials);
                    var content = new StringContent(json, Encoding.UTF8, "application/json");

                    var response = await client.PostAsync($"{baseUrl}/auth/login", content);
                    var responseString = await response.Content.ReadAsStringAsync();

                    if (response.IsSuccessStatusCode)
                    {
                        var result = JsonSerializer.Deserialize<JsonElement>(responseString);
                        currentUserId = result.GetProperty("id").GetInt32();
                        currentUserName = result.GetProperty("name").GetString();

                        tbUserInfo.Text = $"Пользователь: {currentUserName} (ID: {currentUserId})";

                        MessageBox.Show("Вход выполнен успешно!", "Успех",
                            MessageBoxButton.OK, MessageBoxImage.Information);

                        // Загружаем данные после входа
                        await LoadChats();
                        await LoadContacts();
                    }
                    else
                    {
                        MessageBox.Show($"Ошибка входа: {responseString}", "Ошибка",
                            MessageBoxButton.OK, MessageBoxImage.Error);
                    }
                }
                catch (Exception ex)
                {
                    MessageBox.Show($"Ошибка соединения: {ex.Message}", "Ошибка",
                        MessageBoxButton.OK, MessageBoxImage.Error);
                }
            }
        }

        // Получение чатов
        private async void BtnChats_Click(object sender, RoutedEventArgs e)
        {
            if (currentUserId == 0)
            {
                MessageBox.Show("Сначала войдите в систему!", "Ошибка",
                    MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            await LoadChats();
            MainTabControl.SelectedIndex = 1;
        }

        private async Task LoadChats()
        {
            try
            {
                var response = await client.GetAsync($"{baseUrl}/chats/{currentUserId}");
                if (response.IsSuccessStatusCode)
                {
                    var responseString = await response.Content.ReadAsStringAsync();
                    var result = JsonSerializer.Deserialize<JsonElement>(responseString);

                    if (result.TryGetProperty("chats", out var chatsArray))
                    {
                        var chats = new List<Chat>();
                        foreach (var chat in chatsArray.EnumerateArray())
                        {
                            chats.Add(new Chat
                            {
                                Id = chat.GetProperty("id").GetInt32(),
                                Name = chat.GetProperty("name").GetString(),
                                IsGroup = chat.GetProperty("isGroup").GetBoolean(),
                                CreatedBy = chat.GetProperty("createdBy").GetInt32(),
                                CreatedAt = chat.GetProperty("createdAt").GetString()
                            });
                        }

                        dgChats.ItemsSource = chats;
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Ошибка загрузки чатов: {ex.Message}");
            }
        }

        // Получение контактов
        private async void BtnContacts_Click(object sender, RoutedEventArgs e)
        {
            if (currentUserId == 0)
            {
                MessageBox.Show("Сначала войдите в систему!", "Ошибка",
                    MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            await LoadContacts();
            MainTabControl.SelectedIndex = 2;
        }

        private async Task LoadContacts()
        {
            try
            {
                var response = await client.GetAsync($"{baseUrl}/contacts/{currentUserId}");
                if (response.IsSuccessStatusCode)
                {
                    var responseString = await response.Content.ReadAsStringAsync();
                    var result = JsonSerializer.Deserialize<JsonElement>(responseString);

                    if (result.TryGetProperty("contacts", out var contactsArray))
                    {
                        var contacts = new List<Contact>();
                        foreach (var contact in contactsArray.EnumerateArray())
                        {
                            contacts.Add(new Contact
                            {
                                UserId = contact.GetProperty("userId").GetInt32(),
                                Name = contact.GetProperty("name").GetString(),
                                Login = contact.GetProperty("login").GetString()
                            });
                        }

                        dgContacts.ItemsSource = contacts;
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Ошибка загрузки контактов: {ex.Message}");
            }
        }

        // Создание чата
        private async void BtnCreateChat_Click(object sender, RoutedEventArgs e)
        {
            if (currentUserId == 0)
            {
                MessageBox.Show("Сначала войдите в систему!", "Ошибка",
                    MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            var dialog = new CreateChatDialog();
            if (dialog.ShowDialog() == true)
            {
                try
                {
                    var chatData = new
                    {
                        name = dialog.ChatName,
                        isGroup = dialog.IsGroup,
                        createdBy = currentUserId,
                        participants = dialog.Participants.Split(',')
                                    .Where(p => !string.IsNullOrWhiteSpace(p))
                                    .Select(int.Parse)
                                    .ToArray()
                    };

                    var json = JsonSerializer.Serialize(chatData);
                    var content = new StringContent(json, Encoding.UTF8, "application/json");

                    var response = await client.PostAsync($"{baseUrl}/chats", content);
                    var responseString = await response.Content.ReadAsStringAsync();

                    if (response.IsSuccessStatusCode)
                    {
                        MessageBox.Show("Чат создан успешно!", "Успех",
                            MessageBoxButton.OK, MessageBoxImage.Information);
                        await LoadChats();
                    }
                    else
                    {
                        MessageBox.Show($"Ошибка: {responseString}", "Ошибка",
                            MessageBoxButton.OK, MessageBoxImage.Error);
                    }
                }
                catch (Exception ex)
                {
                    MessageBox.Show($"Ошибка: {ex.Message}", "Ошибка",
                        MessageBoxButton.OK, MessageBoxImage.Error);
                }
            }
        }

        // Добавление контакта
        private async void BtnAddContact_Click(object sender, RoutedEventArgs e)
        {
            if (currentUserId == 0)
            {
                MessageBox.Show("Сначала войдите в систему!", "Ошибка",
                    MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            var dialog = new AddContactDialog();
            if (dialog.ShowDialog() == true)
            {
                try
                {
                    var contactData = new
                    {
                        userId1 = currentUserId,
                        userId2 = dialog.FriendId
                    };

                    var json = JsonSerializer.Serialize(contactData);
                    var content = new StringContent(json, Encoding.UTF8, "application/json");

                    var response = await client.PostAsync($"{baseUrl}/contacts", content);
                    var responseString = await response.Content.ReadAsStringAsync();

                    if (response.IsSuccessStatusCode)
                    {
                        MessageBox.Show("Контакт добавлен!", "Успех",
                            MessageBoxButton.OK, MessageBoxImage.Information);
                        await LoadContacts();
                    }
                    else
                    {
                        MessageBox.Show($"Ошибка: {responseString}", "Ошибка",
                            MessageBoxButton.OK, MessageBoxImage.Error);
                    }
                }
                catch (Exception ex)
                {
                    MessageBox.Show($"Ошибка: {ex.Message}", "Ошибка",
                        MessageBoxButton.OK, MessageBoxImage.Error);
                }
            }
        }

        // Отправка сообщения из интерфейса чата
        private async void BtnSend_Click(object sender, RoutedEventArgs e)
        {
            if (currentUserId == 0)
            {
                MessageBox.Show("Сначала войдите в систему!", "Ошибка",
                    MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            if (currentChatId == 0)
            {
                MessageBox.Show("Выберите чат!", "Ошибка",
                    MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            if (string.IsNullOrWhiteSpace(tbMessage.Text))
            {
                MessageBox.Show("Введите сообщение!", "Ошибка",
                    MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            try
            {
                var messageData = new
                {
                    userId = currentUserId,
                    chatId = currentChatId,
                    message = tbMessage.Text
                };

                var json = JsonSerializer.Serialize(messageData);
                var content = new StringContent(json, Encoding.UTF8, "application/json");

                var response = await client.PostAsync($"{baseUrl}/messages", content);
                var responseString = await response.Content.ReadAsStringAsync();

                if (response.IsSuccessStatusCode)
                {
                    // Очищает ввод
                    tbMessage.Text = "";

                    // Обновляем сообщения
                    await LoadMessages(currentChatId);
                }
                else
                {
                    MessageBox.Show($"Ошибка отправки: {responseString}", "Ошибка",
                        MessageBoxButton.OK, MessageBoxImage.Error);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Ошибка: {ex.Message}", "Ошибка",
                    MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        // Выбор чата из DataGrid
        private void DgChats_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (dgChats.SelectedItem is Chat selectedChat)
            {
                currentChatId = selectedChat.Id;
                tbUserInfo.Text = $"Пользователь: {currentUserName} | Чат: {selectedChat.Name} (ID: {selectedChat.Id})";
            }
        }

        // Кнопка открыть в DataGrid
        private async void BtnOpenChat_Click(object sender, RoutedEventArgs e)
        {
            if (sender is Button button && button.Tag is int chatId)
            {
                currentChatId = chatId;

                // Находим выбранный чат по ID
                if (dgChats.ItemsSource is List<Chat> chats)
                {
                    var selectedChat = chats.FirstOrDefault(c => c.Id == chatId);
                    if (selectedChat != null)
                    {
                        tbUserInfo.Text = $"Пользователь: {currentUserName} | Чат: {selectedChat.Name} (ID: {selectedChat.Id})";
                    }
                }

                // Загружаем сообщения
                await LoadMessages(currentChatId);

                MainTabControl.SelectedIndex = 0;
            }
        }

        // Загрузка сообщений чата
        private async Task LoadMessages(int chatId)
        {
            try
            {
                var response = await client.GetAsync($"{baseUrl}/chats/{chatId}/messages");
                if (response.IsSuccessStatusCode)
                {
                    var responseString = await response.Content.ReadAsStringAsync();
                    var result = JsonSerializer.Deserialize<JsonElement>(responseString);

                    if (result.TryGetProperty("messages", out var messagesArray))
                    {
                        var messages = new List<Message>();
                        foreach (var msg in messagesArray.EnumerateArray())
                        {
                            var message = new Message
                            {
                                Id = msg.GetProperty("id").GetInt32(),
                                UserId = msg.GetProperty("userId").GetInt32(),
                                Content = msg.GetProperty("message").GetString(),
                                Timestamp = msg.GetProperty("sendDate").GetString(),
                                UserName = $"User{msg.GetProperty("userId").GetInt32()}"
                            };

                            messages.Add(message);
                        }

                        lbMessages.ItemsSource = messages;

                        // Прокручиваем вниз
                        if (messages.Count > 0)
                        {
                            var lastMessage = messages[messages.Count - 1];
                            lbMessages.ScrollIntoView(lastMessage);
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Ошибка загрузки сообщений: {ex.Message}");
            }
        }

        // Обновление чатов
        private async void BtnRefreshChats_Click(object sender, RoutedEventArgs e)
        {
            await LoadChats();
        }

        // Выбор чата из DataGrid
        private async void DgChats_MouseDoubleClick(object sender, System.Windows.Input.MouseButtonEventArgs e)
        {
            if (dgChats.SelectedItem is Chat selectedChat)
            {
                currentChatId = selectedChat.Id;
                await LoadMessages(currentChatId);
                MainTabControl.SelectedIndex = 0;
            }
        }
    }
}