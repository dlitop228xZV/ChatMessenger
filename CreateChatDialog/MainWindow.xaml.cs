using System.Windows;
using System.Xml.Linq;

namespace ChatClientWPF
{
    public partial class CreateChatDialog : Window
    {
        public string ChatName => tbName.Text;
        public bool IsGroup => cbIsGroup.IsChecked ?? false;
        public string Participants => tbParticipants.Text;

        public CreateChatDialog()
        {
            InitializeComponent();
        }

        private void BtnCreate_Click(object sender, RoutedEventArgs e)
        {
            if (string.IsNullOrWhiteSpace(tbName.Text))
            {
                MessageBox.Show("Введите название чата!", "Ошибка",
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