using System.Windows;

namespace ChatClientWPF
{
    public partial class SendMessageDialog : Window
    {
        public int ChatId
        {
            get
            {
                if (int.TryParse(tbChatId.Text, out int id))
                    return id;
                return 0;
            }
        }

        public string MessageText => tbMessage.Text;

        public SendMessageDialog()
        {
            InitializeComponent();
        }

        private void BtnSend_Click(object sender, RoutedEventArgs e)
        {
            if (string.IsNullOrWhiteSpace(tbChatId.Text) || !int.TryParse(tbChatId.Text, out _))
            {
                MessageBox.Show("Введите корректный ID чата!", "Ошибка",
                    MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            if (string.IsNullOrWhiteSpace(tbMessage.Text))
            {
                MessageBox.Show("Введите сообщение!", "Ошибка",
                    MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            DialogResult = true;
        }

        private void BtnCancel_Click(object sender, RoutedEventArgs e)
        {
            DialogResult = false;
        }
    }
}