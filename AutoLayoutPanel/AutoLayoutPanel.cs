using System;
using System.Collections;
using System.Windows;
using System.Windows.Controls;
using System.Reflection;
using Cassowary;

namespace AutoLayoutPanel
{
    public class AutoLayoutPanel : Panel
    {
        private struct Constraint
        {
            public ClConstraint constraint;
            public ClVariable propertyFirstVariable;
            public ClVariable propertySecondVariable;
            public String propertyFirst;
            public String propertySecond;
            public UIElement controlFirst;
            public UIElement controlSecond;
        };

        private Hashtable VarConstraints;
        private ArrayList Constraints;
        private Hashtable Controls;
        private Hashtable ControlVariables;
        private ClSimplexSolver solver;

        public AutoLayoutPanel() : base() {
            Controls = new Hashtable();
            Constraints = new ArrayList();
            ControlVariables = new Hashtable();
            solver = new ClSimplexSolver();
            VarConstraints = new Hashtable();

            // Register ourselves.
            FindClControlByUIElement(this);

            // Force our X/Y to be 0, 0
            solver.AddConstraint(new ClLinearEquation(
                FindClVariableByUIElementAndProperty(this, "X"), 
                new ClLinearExpression(0.0), 
                ClStrength.Required));

            solver.AddConstraint(new ClLinearEquation(
                FindClVariableByUIElementAndProperty(this, "Y"),
                new ClLinearExpression(0.0),
                ClStrength.Required));
        }

        protected String GetId(UIElement cntl)
        {
            return (String)Controls[cntl];
        }
        
        protected void AddNewControl(UIElement cntl)
        {
            ClVariable clX = FindClVariableByUIElementAndProperty(cntl, "X");
            ClVariable clY = FindClVariableByUIElementAndProperty(cntl, "Y");
            ClVariable clWidth = FindClVariableByUIElementAndProperty(cntl, "Width");
            ClVariable clHeight = FindClVariableByUIElementAndProperty(cntl, "Height");
            ClVariable clLeft = FindClVariableByUIElementAndProperty(cntl, "Left");
            ClVariable clRight = FindClVariableByUIElementAndProperty(cntl, "Right");
            ClVariable clCenter = FindClVariableByUIElementAndProperty(cntl, "Center");
            ClVariable clMiddle = FindClVariableByUIElementAndProperty(cntl, "Middle");
            ClVariable clTop = FindClVariableByUIElementAndProperty(cntl, "Top");
            ClVariable clBottom = FindClVariableByUIElementAndProperty(cntl, "Bottom");

            // X = Left
            solver.AddConstraint(new ClLinearEquation(clX, new ClLinearExpression(clLeft), ClStrength.Required));

            // X = Center - (Width/2)
            solver.AddConstraint(new ClLinearEquation(clX,
                new ClLinearExpression(clCenter).Minus(new ClLinearExpression(clWidth).Divide(2)), ClStrength.Required));

            // X = Right - Width
            solver.AddConstraintNoException(new ClLinearEquation(clX,
                new ClLinearExpression(clRight).Minus(clWidth), ClStrength.Required));

            // Y = Top
            solver.AddConstraint(new ClLinearEquation(clY, new ClLinearExpression(clTop), ClStrength.Required));

            // Y = Middle - (Height/2)
            solver.AddConstraint(new ClLinearEquation(clY,
                new ClLinearExpression(clMiddle).Minus(new ClLinearExpression(clHeight).Divide(2)), ClStrength.Required));

            // Y = Bottom - Height
            solver.AddConstraint(new ClLinearEquation(clY,
                new ClLinearExpression(clBottom).Minus(clHeight), ClStrength.Required));
        }

        protected UIElement FindClControlByUIElement(UIElement em)
        {
            if (!Controls.ContainsKey(em))
            {
                Controls.Add(em, (Guid.NewGuid()).ToString());
                AddNewControl(em);
            }
            return em;
        }

        protected ClVariable FindClVariableByUIElementAndProperty(UIElement em, String property)
        {
            String key = GetId(em) + "_" + property;
            if (!ControlVariables.ContainsKey(key))
                ControlVariables.Add(key, new ClVariable(key));
            return (ClVariable)(ControlVariables[key]);
        }

        public int AddLayoutConstraint(UIElement controlFirst, 
            String propertyFirst,
            String relatedBy, 
            UIElement controlSecond, 
            String propertySecond, 
            double multiplier, 
            double constant) 
        {
            Constraint target = new Constraint();
            target.propertyFirst = propertyFirst;
            target.controlFirst = FindClControlByUIElement(controlFirst);
            target.propertyFirstVariable = FindClVariableByUIElementAndProperty(controlFirst, propertyFirst);

            int ndx = Constraints.Count;
            byte equality = (byte)(relatedBy.Equals("<") ? Cl.LEQ : relatedBy.Equals(">") ? Cl.GEQ : 0);

            if (controlSecond == null) {
                if (equality == 0)
                    target.constraint = new ClLinearEquation(target.propertyFirstVariable, constant, ClStrength.Required);
                else
                    target.constraint = new ClLinearInequality(target.propertyFirstVariable, equality, constant, ClStrength.Required);
            } else {    
                target.controlSecond = FindClControlByUIElement(controlSecond);
                target.propertySecondVariable = FindClVariableByUIElementAndProperty(controlSecond, propertySecond);
                target.propertySecond = propertySecond;

                if (equality == 0) {
                    // y = m*x + c
                    target.constraint = new ClLinearEquation(
                        target.propertyFirstVariable,
                        new ClLinearExpression(target.propertySecondVariable)
                            .Times(multiplier)
                            .Plus(new ClLinearExpression(constant)),
                        ClStrength.Required);
                } else {
                    // y < m*x + c ||  y > m*x + c
                    target.constraint = new ClLinearInequality(
                        target.propertyFirstVariable,
                        equality,
                        new ClLinearExpression(target.propertySecondVariable)
                            .Times(multiplier)
                            .Plus(new ClLinearExpression(constant)),
                        ClStrength.Required);
                }
            }
            solver.AddConstraint(target.constraint);
            return Constraints.Add(target);
        }

        public void RemoveLayoutConstraint(int ndx)
        {
            Constraint c = (Constraint)Constraints[ndx];
            solver.RemoveConstraint(c.constraint);
            //TODO: Determine if target controls need to be in Controls, ControlVariables, VarContraints
            Constraints.RemoveAt(ndx);
        }

        protected void SetValue(ClVariable v, double x, ClStrength s)
        {
            // TODO: Find a better way then manually adding/removing constriants.
            if (VarConstraints.ContainsKey(v.Name))
            {
                ClLinearEquation eq = (ClLinearEquation)VarConstraints[v.Name];
                solver.RemoveConstraint(eq);
                VarConstraints.Remove(v.Name);
            }
            ClLinearEquation eq2 = new ClLinearEquation(v, new ClLinearExpression(x), s);
            solver.AddConstraint(eq2);
            VarConstraints.Add(v.Name, eq2);
        }

        protected override Size MeasureOverride(Size availableSize)
        {
            foreach (UIElement child in InternalChildren)
            {
                if (!child.IsMeasureValid)
                    child.Measure(availableSize);
            }
            return availableSize;
        }

        protected override Size ArrangeOverride(Size finalSize)
        {
            SetValue(FindClVariableByUIElementAndProperty(this, "Width"), 
                finalSize.Width, ClStrength.Required);
            SetValue(FindClVariableByUIElementAndProperty(this, "Height"), 
                finalSize.Height, ClStrength.Required);

            foreach (UIElement child in InternalChildren)
            {
                SetValue(FindClVariableByUIElementAndProperty(child, "Width"), 
                    child.DesiredSize.Width, ClStrength.Required);
                SetValue(FindClVariableByUIElementAndProperty(child, "Height"),
                    child.DesiredSize.Height, ClStrength.Required);
            }

            solver.Resolve();

            foreach (UIElement child in InternalChildren)
            {
                String Id = GetId(child);
                child.Arrange(new Rect(new Point(((ClVariable)ControlVariables[Id + "_X"]).Value,
                                ((ClVariable)ControlVariables[Id + "_Y"]).Value),
                                new Size(((ClVariable)ControlVariables[Id + "_Width"]).Value,
                                ((ClVariable)ControlVariables[Id + "_Height"]).Value))
                );
            }
            return finalSize;
        }
    }
}




//Console.WriteLine("Panel width is: " + finalSize.Width);
//Console.WriteLine("Panel height is: " + finalSize.Height);
//Console.WriteLine(solver.GetDebugInfo());

/*
    Cl.Plus(
        constant,
        Cl.Times(
            multiplier,
            target.propertySecondVariable
        )
    )*/

/*
target.propertyFirstVariable,
Cl.Plus(constant,
    Cl.Times(multiplier, target.propertySecondVariable)
),
ClStrength.Required);*/
/* 
 Controls.Add(this, (Guid.NewGuid().ToString()));
 ClVariable Top = new ClVariable(GetId(this) + "_Top", 0);
 ClVariable Middle = new ClVariable(GetId(this) + "_Middle", 0);
 ClVariable Bottom = new ClVariable(GetId(this) + "_Bottom", 0);
 ClVariable Left = new ClVariable(GetId(this) + "_Left", 0);
 ClVariable Center = new ClVariable(GetId(this) + "_Center", 0);
 ClVariable Right = new ClVariable(GetId(this) + "_Right",0);
 ClVariable Width = new ClVariable(GetId(this) + "_Width",0);
 ClVariable Height = new ClVariable(GetId(this) + "_Height",0);
 ClVariable X = new ClVariable(GetId(this) + "_X",0);
 ClVariable Y = new ClVariable(GetId(this) + "_Y",0);

 ControlVariables.Add(GetId(this) + "_X", X);
 ControlVariables.Add(GetId(this) + "_Y", Y);
 ControlVariables.Add(GetId(this) + "_Width", Width);
 ControlVariables.Add(GetId(this) + "_Height", Height);
 ControlVariables.Add(GetId(this) + "_Top", Top);
 ControlVariables.Add(GetId(this) + "_Middle", Middle);
 ControlVariables.Add(GetId(this) + "_Bottom", Bottom);
 ControlVariables.Add(GetId(this) + "_Left", Left);
 ControlVariables.Add(GetId(this) + "_Center", Center);
 ControlVariables.Add(GetId(this) + "_Right", Right);*/

//solver.AddConstraint(new ClLinearEquation(Width, new ClLinearExpression(1), ClStrength.Required));
//solver.AddConstraint(new ClLinearEquation(Height, new ClLinearExpression(1), ClStrength.Required));
// Y = Top
//solver.AddConstraint(new ClLinearEquation(clY, new ClLinearExpression(clTop), ClStrength.Strong));
// X = Left
//solver.AddConstraint(new ClLinearEquation(clX, new ClLinearExpression(clLeft), ClStrength.Strong));
// Height = Parent.Height - (Y + Bottom)
//if(solver.AddConstraintNoException(new ClLinearEquation(
//    new ClLinearExpression(pHeight).Minus(new ClLinearExpression(clY).Plus(clBottom)),
//    ClStrength.Strong)))
//{
//    Console.WriteLine("failed Height = Parent.Height - (Y + Bottom): \n" + solver.GetDebugInfo());
//}
// Y = Parent.Height - (Height + Bottom)
/*if(solver.AddConstraintNoException(new ClLinearEquation(
    clY,
    new ClLinearExpression(pHeight).Minus(new ClLinearExpression(clHeight).Plus(clBottom)),
    ClStrength.Strong)))
{
    Console.WriteLine("failed Y = Parent.Height - (Height + Bottom): \n" + solver.GetDebugInfo());
}
            
if ()
{
    Console.WriteLine("failed Right = Parent.Width - (X + Width): \n" + solver.GetDebugInfo());
}*/
// Width = Parent.Width - (X + Right)
//if(solver.AddConstraintNoException(new ClLinearEquation(
//    Cl.Minus(new ClLinearExpression(pWidth), Cl.Plus(clX, new ClLinearExpression(clRight))),
//    clWidth,
//    ClStrength.Strong)))
//{
//    Console.WriteLine("failed Width = Parent.Width - (X + Right): \n" + solver.GetDebugInfo());
//}
// X = Parent.Width - (Width + Right)
//if (solver.AddConstraintNoException(new ClLinearEquation(
//    clX,
//    Cl.Minus(new ClLinearExpression(pWidth), Cl.Plus(clWidth, new ClLinearExpression(clRight))),
//    ClStrength.Strong)))
//{
//    Console.WriteLine("failed X = Parent.Width - (Width + Right): \n" + solver.GetDebugInfo());
//}


// X = (parent.Width - (Width + Right))
/*solver.AddConstraint(new ClLinearEquation(clX, 
    Cl.Minus(
        new ClLinearExpression(pWidth),
        Cl.Plus(
            new ClLinearExpression(clWidth),
            new ClLinearExpression(clRight)
        )
    ), 
    ClStrength.Strong));*/
// Y = (parent.Height - (Bottom + Height))
/*solver.AddConstraint(new ClLinearEquation(
    clY,
    Cl.Minus(
        new ClLinearExpression(pHeight),
        Cl.Plus(
            new ClLinearExpression(clBottom),
            new ClLinearExpression(clHeight)
        )
    ),
    ClStrength.Strong
));*/
// Height = Bottom - Top
//solver.AddConstraint(new ClLinearEquation(clHeight, Cl.Minus(new ClLinearExpression(clBottom), new ClLinearExpression(clTop)), ClStrength.Strong));
// Width = Right - Left
//solver.AddConstraint(new ClLinearEquation(clWidth, Cl.Minus(new ClLinearExpression(clRight), new ClLinearExpression(clLeft)), ClStrength.Strong));
// Center = (parent.Width/2 - Width/2)dth
/*solver.AddConstraint(new ClLinearEquation(
    clCenter, 
    Cl.Minus(
        Cl.Divide(new ClLinearExpression(pWidth),new ClLinearExpression(2)),
        Cl.Divide(new ClLinearExpression(clWidth),new ClLinearExpression(2))
    ),
    ClStrength.Strong
));*/

// Intrinsic size as weak.
//solver.AddConstraint(new ClLinearEquation(clWidth, new ClLinearExpression(1), ClStrength.Required));
//solver.AddConstraint(new ClLinearEquation(clHeight, new ClLinearExpression(1), ClStrength.Required));