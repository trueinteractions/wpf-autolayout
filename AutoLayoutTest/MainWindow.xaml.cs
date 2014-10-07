using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace AutoLayoutTest
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
            AutoLayoutPanel.AutoLayoutPanel panel = new AutoLayoutPanel.AutoLayoutPanel();
            this.Content = panel;
            Button button = new Button();
            button.Content = "Foo";
            panel.Children.Add(button);
            panel.AddLayoutConstraint(button, "Middle", "=", panel, "Middle", 1, 0);
            panel.AddLayoutConstraint(button, "Center", "=", panel, "Center", 1, 0);
            //panel.AddLayoutConstraint(button, "Top", "=", panel, "Height", 0.5,  5);
            //panel.AddLayoutConstraint(button, "Left", "=", panel, "Width", 0.25, 5);
            //panel.AddLayoutConstraint(button, "Right", "=", panel, "Width", 0.75,0);
        }
    }
}
