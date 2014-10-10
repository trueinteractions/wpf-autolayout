#using <System.dll>
#using <System.Collections.dll>
#using <System.Windows.dll>
#using <System.Reflection.dll>
#using <WPF/WindowsBase.dll>
#using <WPF/PresentationCore.dll>
#using <WPF/PresentationFramework.dll>

#include "include/Cl.h"

using namespace std;
using namespace System;
using namespace System::Windows;
using namespace System::Collections;

void MarshalString ( String ^ s, string& os ) {
   using namespace Runtime::InteropServices;
   const char* chars = 
      (const char*)(Marshal::StringToHGlobalAnsi(s)).ToPointer();
   os = chars;
   Marshal::FreeHGlobal(IntPtr((void*)chars));
}

namespace AutoLayoutPanel
{
    ref struct Constraint
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
            this->Controls = gcnew Hashtable();
            Constraints = gcnew ArrayList();
            ControlVariables = new std::map<std::string, ClVariable *>();
            solver = new ClSimplexSolver();
            //VarConstraints = gcnew Hashtable();
            VarConstraints = new std::map<std::string, ClLinearEquation *>();

            // Register ourselves.
            FindClControlByUIElement(this);

            // Force our X/Y to be 0, 0
            ClVariable* cv1 = FindClVariableByUIElementAndProperty(this, "X");
            ClLinearEquation* cl1 = new ClLinearEquation(*cv1, ClGenericLinearExpression<double>(0.0));
            solver->AddConstraint(*cl1);

            ClVariable* cv3 = FindClVariableByUIElementAndProperty(this, "Y");
            ClLinearEquation* cl2 = new ClLinearEquation(*cv3, ClGenericLinearExpression<double>(0.0));
            solver->AddConstraint(*cl2);
        }

        String^ GetId(UIElement^ cntl)
        {
            return (String^)this->Controls[cntl];
        }
        
        void AddNewControl(UIElement^ cntl)
        {
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

            // X = Center - (Width/2)
            ClGenericLinearExpression<double> cle3 = (ClGenericLinearExpression<double>(*clWidth)).Divide(2);
            ClGenericLinearExpression<double> cle2 = (ClGenericLinearExpression<double>(*clCenter)).Minus(cle3);
            ClLinearEquation *cl2 = new ClLinearEquation(*clX, cle2, ClsRequired());
            solver->AddConstraint(*cl2);

            // X = Right - Width
            ClGenericLinearExpression<double> cle4 = (ClGenericLinearExpression<double>(*clRight)).Minus(*clWidth);
            ClLinearEquation *cl3 = new ClLinearEquation(*clX, cle4, ClsRequired());
            solver->AddConstraint(*cl3);

            // Y = Top
            ClLinearEquation *cl4 = new ClLinearEquation(*clY, ClGenericLinearExpression<double>(*clTop), ClsRequired());
            solver->AddConstraint(*cl4);

            // Y = Middle - (Height/2)
            ClGenericLinearExpression<double> cle5 = ClGenericLinearExpression<double>(*clMiddle).Minus(ClGenericLinearExpression<double>(*clHeight).Divide(2));
            ClLinearEquation *cl5 = new ClLinearEquation(*clY, cle5, ClsRequired());
            solver->AddConstraint(*cl5);

            // Y = Bottom - Height
            ClGenericLinearExpression<double> cle6 = ClGenericLinearExpression<double>(*clBottom).Minus(*clHeight);
            ClLinearEquation *cl6 = new ClLinearEquation(*clY, cle6, ClsRequired());
            solver->AddConstraint(*cl6);
        }

        UIElement^ FindClControlByUIElement(UIElement^ em)
        {
            if (!Controls->ContainsKey(em))
            {
                this->Controls->Add(em, (Guid::NewGuid()).ToString());
                AddNewControl(em);
            }
            return em;
        }

        ClVariable* FindClVariableByUIElementAndProperty(UIElement^ em, String^ property)
        {
            String^ key = GetId(em) + "_" + property;
            std::string _key;
            MarshalString(key, _key);
            if(ControlVariables->find(_key) == ControlVariables->end()) {
                ControlVariables->insert(std::pair<std::string, ClVariable *>(_key,new ClVariable(_key)));
            //if (!ControlVariables->ContainsKey(key)) {
                //System::Runtime::InteropServices::GCHandle h = System::Runtime::InteropServices::GCHandle::FromIntPtr(IntPtr(new ClVariable(_key)));
                //ControlVariables->Add(key, h.Target);
            }
            return (ClVariable *)ControlVariables->at(_key);
            //return (ClVariable *)(System::Runtime::InteropServices::GCHandle::ToIntPtr(
            //    System::Runtime::InteropServices::GCHandle::Alloc(ControlVariables[key])).ToPointer());
        }

        int AddLayoutConstraint(UIElement^ controlFirst, 
            String^ propertyFirst,
            String^ relatedBy, 
            UIElement^ controlSecond, 
            String^ propertySecond, 
            double multiplier, 
            double constant) 
        {
            Constraint^ target = gcnew Constraint();
            target->propertyFirst = propertyFirst;
            target->controlFirst = FindClControlByUIElement(controlFirst);
            target->propertyFirstVariable = FindClVariableByUIElementAndProperty(controlFirst, propertyFirst);

            int ndx = Constraints->Count;
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
            return Constraints->Add(target);
        }

        void RemoveLayoutConstraint(int ndx)
        {
            Constraint^ c = (Constraint^)Constraints[ndx];
            solver->RemoveConstraint(c->constraint);
            //TODO: Determine if target controls need to be in Controls, ControlVariables, VarContraints
            Constraints->RemoveAt(ndx);
        }

        void SetValue(ClVariable* v, double x, ClStrength s)
        {
            // TODO: Find a better way then manually adding/removing constriants.
            //if (VarConstraints->ContainsKey(v->Name))
            if(VarConstraints->find(v->Name()) == VarConstraints->end())
            {
                ClLinearEquation* eq = (ClLinearEquation *)VarConstraints->at(v->Name());
                solver->RemoveConstraint(eq);
                VarConstraints->erase(v->Name());
            }
            ClLinearEquation* eq2 = new ClLinearEquation(*v, ClGenericLinearExpression<double>(x), s);
            solver->AddConstraint(eq2);
            VarConstraints->insert(std::pair<std::string, ClLinearEquation*>(v->Name(), eq2));
        }

        virtual Size MeasureOverride(Size availableSize) override
        {
            IEnumerator^ e = InternalChildren->GetEnumerator();
            do
            {
                UIElement^ child = (UIElement ^)e->Current;
                if (!child->IsMeasureValid)
                    child->Measure(availableSize);
            } while (e->MoveNext());
            return availableSize;
        }

        virtual Size ArrangeOverride(Size finalSize) override
        {
            SetValue(FindClVariableByUIElementAndProperty(this, "Width"), 
                finalSize.Width, ClsRequired());
            SetValue(FindClVariableByUIElementAndProperty(this, "Height"), 
                finalSize.Height, ClsRequired());

            IEnumerator^ e = InternalChildren->GetEnumerator();
            do
            {
                UIElement^ child = (UIElement ^)e->Current;
                SetValue(FindClVariableByUIElementAndProperty(child, "Width"), 
                    child->DesiredSize.Width, ClsStrong());
                SetValue(FindClVariableByUIElementAndProperty(child, "Height"),
                    child->DesiredSize.Height, ClsStrong());
            } while (e->MoveNext());

            solver->Resolve();

            e = InternalChildren->GetEnumerator();
            do
            {
                UIElement^ child = (UIElement ^)e->Current;
                String^ Id = GetId(child);
                std::string _id;
                MarshalString(Id, _id);
                child->Arrange(Rect(Point(((ClVariable *)ControlVariables->at(_id + "_X"))->Value(),
                                    ((ClVariable *)ControlVariables->at(_id + "_Y"))->Value()),
                                Size(((ClVariable *)ControlVariables->at(_id + "_Width"))->Value(),
                                    ((ClVariable *)ControlVariables->at(_id + "_Height"))->Value())));
            } while (e->MoveNext());
            return finalSize;
        }
    private:
        std::map <std::string, ClLinearEquation *> *VarConstraints;
        std::map <std::string, ClVariable *> *ControlVariables;
        ArrayList^ Constraints;
        Hashtable^ Controls;
        ClSimplexSolver* solver;
    };
}