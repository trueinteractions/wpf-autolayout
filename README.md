<h1>wpf-autolayout</h1>

An implementation of Cassowary contraint based system.  It contains a Windows WPF control called AutoLayoutPanel that allows you to add controls and define their layout in y=m*x + c model. Where y and x are either the Left, Center, Right, Top, Middle, Bottom, Width and Height.

Example usage:
```C#
AutoLayoutPanel panel = new AutoLayoutPanel();
Window.Content = panel;
Button b = new Button();
b.Content = "Button Title";
panel.Children.Add(b);

// Assign the left (X) value to the left panel.
// panel.AddLayoutConstraint(b,"Left",panel,"Left",0,0);

// Vertically and horizontally center:
// panel.AddLayoutConstraint(button, "Middle", "=", panel, "Middle", 1, 0);
// panel.AddLayoutConstraint(button, "Center", "=", panel, "Center", 1, 0);

// Set the width by defining where the left and right values of the button
// should be set.  The button will always have 30 pixel margin on left and right sides.
// panel.AddLayoutConstraint(button, "Left", "=", panel, "Left", 0, 30);
// panel.AddLayoutConstraint(button, "Right", "=", panel, "Right", 0, -30);
```
