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
                    child.DesiredSize.Width, ClStrength.Strong);
                SetValue(FindClVariableByUIElementAndProperty(child, "Height"),
                    child.DesiredSize.Height, ClStrength.Strong);
            }

            solver.Resolve();

            foreach (UIElement child in InternalChildren)
            {
                String Id = GetId(child);
                child.Arrange(new Rect(
                                new Point(((ClVariable)ControlVariables[Id + "_X"]).Value,
                                    ((ClVariable)ControlVariables[Id + "_Y"]).Value),
                                new Size(((ClVariable)ControlVariables[Id + "_Width"]).Value,
                                    ((ClVariable)ControlVariables[Id + "_Height"]).Value)));
            }
            return finalSize;
        }
    }
}