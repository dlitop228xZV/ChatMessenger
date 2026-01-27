using System.Windows;

namespace ChatClientWPF
{
    public partial class AddContactDialog : Window
    {
        public int FriendId
        {
            get
            {
                if (int.TryParse(tbFriendId.Text, out int id))
                    return id;
                return 0;
            }
        }

        public AddContactDialog()
        {
            InitializeComponent();
        }

        private void BtnAdd_Click(object sender, RoutedEventArgs e)
        {
            if (string.IsNullOrWhiteSpace(tbFriendId.Text) || !int.TryParse(tbFriendId.Text, out _))
            {
                MessageBox.Show("Введите корректный ID пользователя!", "Ошибка",
                    MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            DialogResult = true;
        }

        private void BtnSearch_Click(object sender, RoutedEventArgs e)
        {
            // Здесь можно реализовать поиск пользователя по логину
            MessageBox.Show("Поиск пользователя (нужно реализовать API запрос)",
                "Информация", MessageBoxButton.OK, MessageBoxImage.Information);
        }

        private void BtnCancel_Click(object sender, RoutedEventArgs e)
        {
            DialogResult = false;
        }
    }
}