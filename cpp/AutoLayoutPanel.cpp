#using <System.dll>
#using <System.Collections.dll>
#using <System.Windows.dll>
#using <System.Reflection.dll>
#using <WPF/WindowsBase.dll>
#using <WPF/PresentationCore.dll>
#using <WPF/PresentationFramework.dll>

#include "include/Cl.h"
// #define PRINT_DEBUG 1

using namespace std;
using namespace System;
using namespace System::Windows;
using namespace System::Collections;

static std::string cnvclrstring( System::String ^ s )
{
    const char* cstr = (const char*) (System::Runtime::InteropServices::Marshal::StringToHGlobalAnsi(s)).ToPointer();
    std::string sstr = cstr;
    System::Runtime::InteropServices::Marshal::FreeHGlobal(System::IntPtr((void*)cstr));
    return sstr;
}

namespace AutoLayout
{
    public ref struct Constraint
    {
        ClConstraint* constraint;
        ClVariable* propertyFirstVariable;
        ClVariable* propertySecondVariable;
        String^ propertyFirst;
        String^ propertySecond;
        UIElement^ controlFirst;
        UIElement^ controlSecond;
    };

    public ref class AutoLayoutPanel : System::Windows::Controls::Panel
    {
    public:
        AutoLayoutPanel() : Panel() {
            Constraints = gcnew ArrayList();
            ControlVariables = gcnew Hashtable();
            solver = new ClSimplexSolver();
            VarConstraints = gcnew Hashtable();
            Controls = gcnew Hashtable();

            // Register ourselves.
            FindClControlByUIElement(this);

            // Force our X/Y to be 0, 0
            ClVariable* cv1 = FindClVariableByUIElementAndProperty(this, "X");
            ClLinearEquation* cl1 = new ClLinearEquation(*cv1, ClGenericLinearExpression<double>(0.0), ClsRequired());
            solver->AddConstraint(*cl1);

            ClVariable* cv3 = FindClVariableByUIElementAndProperty(this, "Y");
            ClLinearEquation* cl2 = new ClLinearEquation(*cv3, ClGenericLinearExpression<double>(0.0), ClsRequired());
            solver->AddConstraint(*cl2);
        }

        String^ GetId(UIElement^ cntl)
        {
            // This needs to run to ensure the element has been added to the
            // autolayout process, even if it wasn't explicitly by the user.
            FindClControlByUIElement(cntl);

            return cntl->Uid;
        }
        
        void AddNewControl(UIElement^ cntl)
        {
#ifdef PRINT_DEBUG
            Console::WriteLine("Adding new control to "+this->Uid+" as "+cntl->Uid+"//"+cntl);
#endif
            ClVariable* clX = FindClVariableByUIElementAndProperty(cntl, "X");
            ClVariable* clY = FindClVariableByUIElementAndProperty(cntl, "Y");
            ClVariable* clWidth = FindClVariableByUIElementAndProperty(cntl, "Width");
            ClVariable* clHeight = FindClVariableByUIElementAndProperty(cntl, "Height");
            ClVariable* clLeft = FindClVariableByUIElementAndProperty(cntl, "Left");
            ClVariable* clRight = FindClVariableByUIElementAndProperty(cntl, "Right");
            ClVariable* clCenter = FindClVariableByUIElementAndProperty(cntl, "Center");
            ClVariable* clMiddle = FindClVariableByUIElementAndProperty(cntl, "Middle");
            ClVariable* clTop = FindClVariableByUIElementAndProperty(cntl, "Top");
            ClVariable* clBottom = FindClVariableByUIElementAndProperty(cntl, "Bottom");

            // X = Left
            ClLinearEquation *cl1 = new ClLinearEquation(*clX, ClGenericLinearExpression<double>(*clLeft), ClsRequired());
            solver->AddConstraint(cl1);
            // Add a preference to keep X above 0.
            ClLinearInequality* cl1a = new ClLinearInequality(*clX, cnGEQ, 0.0, ClsMedium());
            solver->AddConstraint(*cl1a);
            // default to upper left hand corner if nothing else is specified.
            ClLinearEquation* cl2a = new ClLinearEquation(*clX, ClGenericLinearExpression<double>(0.0), ClsWeak());
            solver->AddConstraint(*cl2a);

            // X = Center - (Width/2)
            ClGenericLinearExpression<double> cle3 = (ClGenericLinearExpression<double>(*clWidth)).Divide(2);
            ClGenericLinearExpression<double> cle2 = (ClGenericLinearExpression<double>(*clCenter)).Minus(cle3);
            ClLinearEquation *cl2 = new ClLinearEquation(*clX, cle2, ClsRequired());
            solver->AddConstraint(*cl2);

            // Left = Right - Width
            ClGenericLinearExpression<double> cle4 = (ClGenericLinearExpression<double>(*clRight)).Minus(*clWidth);
            ClLinearEquation *cl3 = new ClLinearEquation(*clLeft, cle4, ClsRequired());
            solver->AddConstraint(*cl3);
            // Add a preference to keep Right above 0.
            ClLinearInequality* cl3a = new ClLinearInequality(*clRight, cnGEQ, 0.0, ClsMedium());
            solver->AddConstraint(*cl3a);

            // Y = Top
            ClLinearEquation *cl4 = new ClLinearEquation(*clY, ClGenericLinearExpression<double>(*clTop), ClsRequired());
            solver->AddConstraint(*cl4);
            // Add a preference to keep Y above 0.
            ClLinearInequality* cl4a = new ClLinearInequality(*clY, cnGEQ, 0.0, ClsMedium());
            solver->AddConstraint(*cl4a);
            // default to upper left hand corner if nothing else is specified.
            ClLinearEquation* cl4b = new ClLinearEquation(*clY, ClGenericLinearExpression<double>(0.0), ClsWeak());
            solver->AddConstraint(*cl4b);

            // Y = Middle - (Height/2)
            ClGenericLinearExpression<double> cle5 = ClGenericLinearExpression<double>(*clMiddle)
                .Minus(ClGenericLinearExpression<double>(*clHeight).Divide(2));
            ClLinearEquation *cl5 = new ClLinearEquation(*clY, cle5, ClsRequired());
            solver->AddConstraint(*cl5);

            // Y = Bottom - Height
            ClGenericLinearExpression<double> cle6 = ClGenericLinearExpression<double>(*clBottom).Minus(*clHeight);
            ClLinearEquation *cl6 = new ClLinearEquation(*clTop, cle6, ClsRequired());
            solver->AddConstraint(*cl6);
        }

        UIElement^ FindClControlByUIElement(UIElement^ em)
        {
            if(!Controls->ContainsKey(em->Uid)) {
                if(em->Uid == "") em->Uid = (Guid::NewGuid()).ToString();
                Controls->Add(em->Uid,em);
                AddNewControl(em);
            }
            return em;
        }

        ClVariable* FindClVariableByUIElementAndProperty(UIElement^ em, String^ property)
        {
            String^ key = GetId(em) + "_" + property;
            if (!ControlVariables->ContainsKey(key))
                ControlVariables->Add(key, gcnew IntPtr(new ClVariable(cnvclrstring(key))));
            return (ClVariable *)((IntPtr ^)ControlVariables[key])->ToPointer();
        }

        Constraint^ AddLayoutConstraint(UIElement^ controlFirst, 
            String^ propertyFirst,
            String^ relatedBy, 
            UIElement^ controlSecond, 
            String^ propertySecond, 
            double multiplier, 
            double constant) 
        {
#ifdef PRINT_DEBUG
            System::Console::WriteLine(this->Uid+": ["+controlFirst+"]."+propertyFirst+" "+relatedBy+" ["+controlSecond+"]."+propertySecond+" * "+multiplier+" + "+constant);
#endif
            Constraint^ target = gcnew Constraint();
            target->propertyFirst = propertyFirst;
            target->controlFirst = FindClControlByUIElement(controlFirst);
            target->propertyFirstVariable = FindClVariableByUIElementAndProperty(controlFirst, propertyFirst);

            ClCnRelation equality = (relatedBy->Equals("<") ? cnLEQ : relatedBy->Equals(">") ? cnGEQ : cnEQ);

            if (controlSecond == nullptr) {
                if (equality == cnEQ)
                    target->constraint = new ClLinearEquation(*(target->propertyFirstVariable), constant, ClsRequired());
                else
                    target->constraint = new ClLinearInequality(*(target->propertyFirstVariable), equality, constant, ClsRequired());
            } else {    
                target->controlSecond = FindClControlByUIElement(controlSecond);
                target->propertySecondVariable = FindClVariableByUIElementAndProperty(controlSecond, propertySecond);
                target->propertySecond = propertySecond;

                if (equality == cnEQ) {
                    // y = m*x + c
                    target->constraint = new ClLinearEquation(
                        *(target->propertyFirstVariable),
                        ClGenericLinearExpression<double>(*(target->propertySecondVariable))
                            .Times(multiplier)
                            .Plus(ClGenericLinearExpression<double>(constant)), ClsRequired());
                } else {
                    // y < m*x + c ||  y > m*x + c
                    target->constraint = new ClLinearInequality(
                        *(target->propertyFirstVariable),
                        equality,
                        ClGenericLinearExpression<double>(*(target->propertySecondVariable))
                            .Times(multiplier)
                            .Plus(ClGenericLinearExpression<double>(constant)), ClsRequired());
                }
            }
            solver->AddConstraint(*(target->constraint));
            this->InvalidateMeasure();
            this->InvalidateArrange();
            this->UpdateLayout();
            return target;
        }

        void RemoveLayoutConstraint(Constraint^ c)
        {
            solver->RemoveConstraint(*(c->constraint));
            this->InvalidateMeasure();
            this->InvalidateArrange();
            this->UpdateLayout();
        }

        void SetPropValue(UIElement^ em, String ^property, ClVariable* v, double x, ClStrength s)
        {
            if(x == System::Double::PositiveInfinity) return;

            String^ key = GetId(em) + "_" + property;

            if(VarConstraints->ContainsKey(key))
            {
                ClLinearEquation* eq = (ClLinearEquation *)((IntPtr ^)VarConstraints[key])->ToPointer();
                if(x == eq->Expression().Constant()) return;
#ifdef PRINT_DEBUG
                System::Console::WriteLine(this->Uid+": ["+em->Uid+"//"+em+"]."+property+" changing from "+eq->Expression().Constant()+" to "+x);
#endif
                solver->RemoveConstraint(eq);
                eq->ChangeConstant(x);
                solver->AddConstraint(eq);
            } else {
#ifdef PRINT_DEBUG
                System::Console::WriteLine(this->Uid+": ["+em->Uid+"//"+em+"]."+property+" setting to "+x);
#endif

                ClLinearEquation* eq2 = new ClLinearEquation(*v, ClGenericLinearExpression<double>(x), s);
                solver->AddConstraint(eq2);
                VarConstraints->Add(key, gcnew IntPtr(eq2));
            }
        }

        Size LayoutPass(Size finalSize, bool measure, bool arrange) {
            SetPropValue(this, "Width", FindClVariableByUIElementAndProperty(this, "Width"), 
                finalSize.Width, ClsRequired());
            SetPropValue(this, "Height", FindClVariableByUIElementAndProperty(this, "Height"), 
                finalSize.Height, ClsRequired());

            double maxY = finalSize.Height == System::Double::PositiveInfinity ? 0.0 : finalSize.Height;
            double maxX = finalSize.Width == System::Double::PositiveInfinity ? 0.0 : finalSize.Width;
            if(InternalChildren->Count > 0) {
                for each(UIElement^ child in InternalChildren)
                {
                    if (!child->IsMeasureValid) {
                        if(measure)
                            child->Measure(finalSize);
#ifdef PRINT_DEBUG
                        System::Console::WriteLine(this->Uid+": ["+child->Uid+"//"+child+"]->DesiredSize = "+child->DesiredSize.Width+", "+child->DesiredSize.Height);
#endif
                        SetPropValue(child, "Width",FindClVariableByUIElementAndProperty(child, "Width"), 
                            child->DesiredSize.Width, ClsMedium());
                        SetPropValue(child, "Height",FindClVariableByUIElementAndProperty(child, "Height"),
                            child->DesiredSize.Height, ClsMedium());
                    }
                }
                solver->Solve();

                for each(UIElement^ child in InternalChildren) {
                    String^ Id = GetId(child);
                    double x = ((ClVariable *)((IntPtr ^)ControlVariables[Id + "_X"])->ToPointer())->Value();
                    double y = ((ClVariable *)((IntPtr ^)ControlVariables[Id + "_Y"])->ToPointer())->Value();
                    double w = ((ClVariable *)((IntPtr ^)ControlVariables[Id + "_Width"])->ToPointer())->Value();
                    double h = ((ClVariable *)((IntPtr ^)ControlVariables[Id + "_Height"])->ToPointer())->Value();

                    if(w < 0) w = 0;
                    if(h < 0) h = 0;

                    if(arrange) {
                        if(child->GetType()->GetProperty("Width") != nullptr) {
                            ((System::Windows::FrameworkElement ^)child)->Width = w;
                            ((System::Windows::FrameworkElement ^)child)->Height = h;
                        }
#ifdef PRINT_DEBUG
                        System::Console::WriteLine(this->Uid+": ["+child+"]->Arrange("+x+","+y+","+w+","+h+")");
#endif
                        child->Arrange(Rect(Point(x, y),  Size(w, h)));
                    }
                    maxX = maxX < (x+w) ? (x+w) : maxX;
                    maxY = maxY < (y+h) ? (y+h) : maxY;
                }
            }
            return Size(maxX, maxY);
        }

        virtual Size MeasureOverride(Size availableSize) override
        {
            Size calcSize = availableSize;
            if(calcSize.Width == System::Double::PositiveInfinity ||
                calcSize.Height == System::Double::PositiveInfinity ) 
            {
#ifdef PRINT_DEBUG
                System::Console::WriteLine(this->Uid+" ---- positive infinity found, going into layout pass ----");
#endif
                calcSize = LayoutPass(availableSize, true, false);
            } else {
                for each (UIElement^ child in InternalChildren)
                {
                    if (!child->IsMeasureValid) {
                        child->Measure(availableSize);
                        SetPropValue(child, "Width",FindClVariableByUIElementAndProperty(child, "Width"), 
                            child->DesiredSize.Width, ClsMedium());
                        SetPropValue(child, "Height",FindClVariableByUIElementAndProperty(child, "Height"),
                            child->DesiredSize.Height, ClsMedium());
                    }
                }
            }
#ifdef PRINT_DEBUG
            System::Console::WriteLine(this->Uid+": Measure("+availableSize+") -> ("+calcSize+")");
#endif
            return calcSize;
        }

        virtual Size ArrangeOverride(Size finalSize) override
        {
            Size setSize = LayoutPass(finalSize, false, true);
#ifdef PRINT_DEBUG
            System::Console::WriteLine(this->Uid+": Arrange("+finalSize+") -> ("+setSize+")");
#endif
            return setSize;
        }
    private:
        Hashtable^ Controls;
        Hashtable^ VarConstraints;
        Hashtable^ ControlVariables;
        ArrayList^ Constraints;
        ClSimplexSolver* solver;
    };
}