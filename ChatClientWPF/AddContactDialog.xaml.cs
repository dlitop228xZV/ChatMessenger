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
    public partial class AddContactDialog : Window
    {
        private HttpClient client = new HttpClient();
        private string baseUrl = "http://localhost:18080";
        private int selectedUserId = 0;
        private string selectedUserLogin = "";

        // Модель пользователя для поиска
        public class UserSearchResult
        {
            public int Id { get; set; }
            public string Name { get; set; }
            public string Login { get; set; }
        }

        public int FriendId => selectedUserId;
        public string FriendLogin => selectedUserLogin;

        public AddContactDialog()
        {
            InitializeComponent();
            client.Timeout = TimeSpan.FromSeconds(30);
        }

        private async void BtnSearch_Click(object sender, RoutedEventArgs e)
        {
            string login = tbLogin.Text.Trim();

            if (string.IsNullOrWhiteSpace(login))
            {
                MessageBox.Show("Введите логин пользователя для поиска!",
                    "Ошибка", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            try
            {
                // Показываем процесс поиска
                tbSearchResult.Text = "Поиск пользователя...";
                lvUsers.ItemsSource = null;
                btnAdd.IsEnabled = false;

                // Реальный запрос к серверу
                var response = await client.GetAsync($"{baseUrl}/users/search/{login}");

                if (response.IsSuccessStatusCode)
                {
                    var responseString = await response.Content.ReadAsStringAsync();
                    var result = JsonSerializer.Deserialize<JsonElement>(responseString);

                    if (result.TryGetProperty("users", out var usersArray))
                    {
                        var users = new List<UserSearchResult>();
                        foreach (var user in usersArray.EnumerateArray())
                        {
                            users.Add(new UserSearchResult
                            {
                                Id = user.GetProperty("id").GetInt32(),
                                Name = user.GetProperty("name").GetString(),
                                Login = user.GetProperty("login").GetString()
                            });
                        }

                        if (users.Count > 0)
                        {
                            lvUsers.ItemsSource = users;
                            tbSearchResult.Text = $"Найдено {users.Count} пользователь(ей)";
                        }
                        else
                        {
                            tbSearchResult.Text = "Пользователи не найдены";
                            lvUsers.ItemsSource = null;
                        }
                    }
                    else
                    {
                        tbSearchResult.Text = "Пользователи не найдены";
                    }
                }
                else
                {
                    tbSearchResult.Text = "Ошибка при поиске пользователей";
                    MessageBox.Show($"Ошибка сервера: {response.StatusCode}",
                        "Ошибка", MessageBoxButton.OK, MessageBoxImage.Error);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Ошибка поиска: {ex.Message}",
                    "Ошибка", MessageBoxButton.OK, MessageBoxImage.Error);
                tbSearchResult.Text = "Ошибка при поиске";
            }
        }

        private void BtnAdd_Click(object sender, RoutedEventArgs e)
        {
            if (lvUsers.SelectedItem is UserSearchResult selectedUser)
            {
                selectedUserId = selectedUser.Id;
                selectedUserLogin = selectedUser.Login;
                DialogResult = true;
            }
            else
            {
                MessageBox.Show("Выберите пользователя из списка!",
                    "Ошибка", MessageBoxButton.OK, MessageBoxImage.Warning);
            }
        }

        private void BtnCancel_Click(object sender, RoutedEventArgs e)
        {
            DialogResult = false;
        }
    }
}