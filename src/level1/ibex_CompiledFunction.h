//============================================================================
//                                  I B E X                                   
// File        : Compiled function
// Author      : Gilles Chabert
// Copyright   : Ecole des Mines de Nantes (France)
// License     : See the LICENSE file
// Created     : Dec 31, 2011
// Last Update : 
//============================================================================

#ifndef _IBEX_COMPILED_FUNCTION_H_
#define _IBEX_COMPILED_FUNCTION_H_

#include <stack>

#include "ibex_Expr.h"
#include "ibex_Function.h"
#include "ibex_FunctionVisitor.h"
#include "ibex_FwdAlgorithm.h"
#include "ibex_BwdAlgorithm.h"
#include "ibex_Decorator.h"

namespace ibex {

/**
 * \ingroup level1
 * \brief A low-level representation of a function for speeding up forward/backward algorithms.
 *
 */
template<typename T>
class CompiledFunction : public FunctionVisitor {
public:
	/**
	 * Create a compiled version of the function \a f, where
	 * each node is decorated with an object of type "T" via the decorator \a d.
	 */
	CompiledFunction(const Function& f, const Decorator<T>& d) : expr(f.expr()), f(f) {
		d.decorate(f);
		code=new operation[expr.size];
		args=new ExprLabel**[expr.size];
		nodes=new const ExprNode*[expr.size];
		nb_args=new int[expr.size];
		ptr=0;
		visit(expr);
	}

	/**
	 * Run the forward phase of a forward algorithm and
	 * return a reference to the label
	 * of the root node. V must be a subclass of FwdAlgorithm<T>.
	 * Note that the type V is just passed in order to have static linkage.
	 */
	template<class V>
	T& forward(const V& algo) const;

	/**
	 * Run the backward phase.  V must be a subclass of BwdAlgorithm<T>.
	 * Note that the type V is just passed in order to have static linkage.
	 */
	template<class V>
	void backward(const V& algo) const;

	/**
	 * \brief The root node of the function expression
	 */
	const ExprNode& expr;

	/**
	 * The function
	 */
	const Function& f;

protected:
	typedef enum {
		IDX, VEC, SYM, CST, APPLY,
		ADD, MUL, SUB, DIV, MAX, MIN, ATAN2,
		MINUS, SIGN, ABS, POWER,
		SQR, SQRT, EXP, LOG,
		COS,  SIN,  TAN,  ACOS,  ASIN,  ATAN,
		COSH, SINH, TANH, ACOSH, ASINH, ATANH,

		ADD_V, ADD_M, SUB_V, SUB_M,
		MUL_SV, MUL_SM, MUL_VV, MUL_MV, MUL_MM
	} operation;

private:

	void visit(const ExprNode& e) {
		e.acceptVisitor(*this);
	}

	void visit(const ExprIndex& i) {
		code[ptr]=IDX;
		nodes[ptr]=&i;
		nb_args[ptr]=1;
		args[ptr]=new ExprLabel*[2];
		args[ptr][0]=i.deco;
		args[ptr][1]=i.expr.deco;

		ptr++;
		visit(i.expr);
	}

	void visit(const ExprSymbol& v) {
		code[ptr]=SYM;
		nodes[ptr]=&v;
		nb_args[ptr]=0;
		args[ptr]=new ExprLabel*[1]; // the unique argument of a Variable is the corresponding index in "csts"
		args[ptr][0]=v.deco;

		ptr++;
	}

	void visit(const ExprConstant& c) {
		code[ptr]=CST;

		nodes[ptr]=&c;
		nb_args[ptr]=0;
		args[ptr]=new ExprLabel*[1];
		args[ptr][0]=c.deco;

		ptr++;
	}

	void visit(const ExprNAryOp& e) {
		e.acceptVisitor(*this);
	}

	void visit(const ExprBinaryOp& b) {
		b.acceptVisitor(*this);
	}

	void visit(const ExprUnaryOp& u) {
		u.acceptVisitor(*this);
	}

	void visit(const ExprNAryOp& e, operation op) {
		code[ptr]=op;
		nodes[ptr]=&e;
		nb_args[ptr]=e.nb_args;
		args[ptr]=new ExprLabel*[e.nb_args+1];
		args[ptr][0]=e.deco;
		for (int i=1; i<e.nb_args; i++)
			args[ptr][i]=e.arg(i).deco;

		ptr++;
		for (int i=0; i<e.nb_args; i++) {
			visit(e.arg(i));
		}
	}

	void visit(const ExprBinaryOp& b, operation op) {
		code[ptr]=op;
		nodes[ptr]=&b;
		nb_args[ptr]=2;
		args[ptr]=new ExprLabel*[3];
		args[ptr][0]=b.deco;
		args[ptr][1]=b.left.deco;
		args[ptr][2]=b.right.deco;

		ptr++;
		visit(b.left);
		visit(b.right);
	}

	void visit(const ExprUnaryOp& u, operation op) {
		code[ptr]=op;		nodes[ptr]=&u;
		nb_args[ptr]=1;
		args[ptr]=new ExprLabel*[2];
		args[ptr][0]=u.deco;
		args[ptr][1]=u.expr.deco;

		ptr++;
		visit(u.expr);
	}

	void visit(const ExprVector& e) { visit(e,VEC); }

	void visit(const ExprApply& e) { visit(e,APPLY); }

	void visit(const ExprAdd& e)   {
		if (e.dim.is_scalar())      visit(e,ADD);
		else if (e.dim.is_vector()) visit(e,ADD_V);
		else                        visit(e,ADD_M);
	}

	void visit(const ExprMul& e)   {
		if (e.left.dim.is_scalar())
			if (e.right.dim.is_scalar())      visit(e,MUL);
			else if (e.right.dim.is_vector()) visit(e,MUL_SV);
			else                              visit(e,MUL_SM);
		else if (e.left.dim.is_vector())      visit(e,MUL_VV);
		else if (e.right.dim.is_vector())     visit(e,MUL_MV);
			else                              visit(e,MUL_MM);
	}

	void visit(const ExprSub& e)   {
		if (e.dim.is_scalar())      visit(e,SUB);
		else if (e.dim.is_vector()) visit(e,SUB_V);
		else                        visit(e,SUB_M);
	}

	void visit(const ExprDiv& e)   { visit(e,DIV); }

	void visit(const ExprMax& e)   { visit(e,MAX); }

	void visit(const ExprMin& e)   { visit(e,MIN); }

	void visit(const ExprAtan2& e) { visit(e,ATAN2); }

	void visit(const ExprMinus& e) { visit(e,MINUS); }

	void visit(const ExprSign& e)  { visit(e,SIGN); }

	void visit(const ExprAbs& e)   { visit(e,ABS); }

	void visit(const ExprPower& e) { visit(e,POWER); }

	void visit(const ExprSqr& e)   { visit(e,SQR); }

	void visit(const ExprSqrt& e)  { visit(e,SQRT); }

	void visit(const ExprExp& e)   { visit(e,EXP); }

	void visit(const ExprLog& e)   { visit(e,LOG); }

	void visit(const ExprCos& e)   { visit(e,COS); }

	void visit(const ExprSin& e)   { visit(e,SIN); }

	void visit(const ExprTan& e)   { visit(e,TAN); }

	void visit(const ExprCosh& e)  { visit(e,COSH); }

	void visit(const ExprSinh& e)  { visit(e,SINH); }

	void visit(const ExprTanh& e)  { visit(e,TANH); }

	void visit(const ExprAcos& e)  { visit(e,ACOS); }

	void visit(const ExprAsin& e)  { visit(e,ASIN); }

	void visit(const ExprAtan& e)  { visit(e,ATAN); }

	void visit(const ExprAcosh& e) { visit(e,ACOSH); }

	void visit(const ExprAsinh& e) { visit(e,ASINH); }

	void visit(const ExprAtanh& e) { visit(e,ATANH); }

protected:

	const char* op(operation o) const;

	template<typename _T>
	friend std::ostream& operator<<(std::ostream&,const CompiledFunction<_T>&);

	const ExprNode** nodes;
	operation *code;
	int* nb_args;
	mutable ExprLabel*** args;

	mutable int ptr;
};

template<typename T>template<class V>
T& CompiledFunction<T>::forward(const V& algo) const {
	assert(dynamic_cast<const FwdAlgorithm<T>* >(&algo)!=NULL);

	for (int i=expr.size-1; i>=0; i--) {
		switch(code[i]) {
		case IDX:    ((V&) algo).index_fwd((ExprIndex&)    *(nodes[i]), (T&) *args[i][1],                   (T&) *args[i][0]); break;
		case VEC:    ((V&) algo).vector_fwd((ExprVector&)  *(nodes[i]), (const T**) &(args[i][1]),          (T&) *args[i][0]); break;
		case SYM:    ((V&) algo).symbol_fwd((ExprSymbol&)  *(nodes[i]),                                     (T&) *args[i][0]); break;
		case CST:    ((V&) algo).cst_fwd  ((ExprConstant&) *(nodes[i]),                                     (T&) *args[i][0]); break;
		case APPLY:  ((V&) algo).apply_fwd((ExprApply&)    *(nodes[i]), (const T**) &(args[i][1]),          (T&) *args[i][0]); break;
		case ADD:    ((V&) algo).add_fwd  ((ExprAdd&)      *(nodes[i]), (T&) *args[i][1], (T&) *args[i][2], (T&) *args[i][0]); break;
		case ADD_V:  ((V&) algo).add_V_fwd  ((ExprAdd&)    *(nodes[i]), (T&) *args[i][1], (T&) *args[i][2], (T&) *args[i][0]); break;
		case ADD_M:  ((V&) algo).add_M_fwd  ((ExprAdd&)    *(nodes[i]), (T&) *args[i][1], (T&) *args[i][2], (T&) *args[i][0]); break;
		case MUL:    ((V&) algo).mul_fwd  ((ExprMul&)      *(nodes[i]), (T&) *args[i][1], (T&) *args[i][2], (T&) *args[i][0]); break;
		case MUL_SV: ((V&) algo).mul_SV_fwd ((ExprMul&)    *(nodes[i]), (T&) *args[i][1], (T&) *args[i][2], (T&) *args[i][0]); break;
		case MUL_SM: ((V&) algo).mul_SM_fwd ((ExprMul&)    *(nodes[i]), (T&) *args[i][1], (T&) *args[i][2], (T&) *args[i][0]); break;
		case MUL_VV: ((V&) algo).mul_VV_fwd ((ExprMul&)    *(nodes[i]), (T&) *args[i][1], (T&) *args[i][2], (T&) *args[i][0]); break;
		case MUL_MV: ((V&) algo).mul_MV_fwd ((ExprMul&)    *(nodes[i]), (T&) *args[i][1], (T&) *args[i][2], (T&) *args[i][0]); break;
		case MUL_MM: ((V&) algo).mul_MM_fwd ((ExprMul&)    *(nodes[i]), (T&) *args[i][1], (T&) *args[i][2], (T&) *args[i][0]); break;
		case SUB:    ((V&) algo).sub_fwd  ((ExprSub&)      *(nodes[i]), (T&) *args[i][1], (T&) *args[i][2], (T&) *args[i][0]); break;
		case SUB_V:  ((V&) algo).sub_V_fwd  ((ExprSub&)    *(nodes[i]), (T&) *args[i][1], (T&) *args[i][2], (T&) *args[i][0]); break;
		case SUB_M:  ((V&) algo).sub_M_fwd  ((ExprSub&)    *(nodes[i]), (T&) *args[i][1], (T&) *args[i][2], (T&) *args[i][0]); break;
		case DIV:    ((V&) algo).div_fwd  ((ExprDiv&)      *(nodes[i]), (T&) *args[i][1], (T&) *args[i][2], (T&) *args[i][0]); break;
		case MAX:    ((V&) algo).max_fwd  ((ExprMax&)      *(nodes[i]), (T&) *args[i][1], (T&) *args[i][2], (T&) *args[i][0]); break;
		case MIN:    ((V&) algo).min_fwd  ((ExprMin&)      *(nodes[i]), (T&) *args[i][1], (T&) *args[i][2], (T&) *args[i][0]); break;
		case ATAN2:  ((V&) algo).atan2_fwd((ExprAtan2&)    *(nodes[i]), (T&) *args[i][1], (T&) *args[i][2], (T&) *args[i][0]); break;
		case MINUS:  ((V&) algo).minus_fwd((ExprMinus&)    *(nodes[i]), (T&) *args[i][1],                   (T&) *args[i][0]); break;
		case SIGN:   ((V&) algo).sign_fwd ((ExprSign&)     *(nodes[i]), (T&) *args[i][1],                   (T&) *args[i][0]); break;
		case ABS:    ((V&) algo).abs_fwd  ((ExprAbs&)      *(nodes[i]), (T&) *args[i][1],                   (T&) *args[i][0]); break;
		case POWER:  ((V&) algo).power_fwd((ExprPower&)    *(nodes[i]), (T&) *args[i][1],                   (T&) *args[i][0]); break;
		case SQR:    ((V&) algo).sqr_fwd  ((ExprSqr&)      *(nodes[i]), (T&) *args[i][1],                   (T&) *args[i][0]); break;
		case SQRT:   ((V&) algo).sqrt_fwd ((ExprSqrt&)     *(nodes[i]), (T&) *args[i][1],                   (T&) *args[i][0]); break;
		case EXP:    ((V&) algo).exp_fwd  ((ExprExp&)      *(nodes[i]), (T&) *args[i][1],                   (T&) *args[i][0]); break;
		case LOG:    ((V&) algo).log_fwd  ((ExprLog&)      *(nodes[i]), (T&) *args[i][1],                   (T&) *args[i][0]); break;
		case COS:    ((V&) algo).cos_fwd  ((ExprCos&)      *(nodes[i]), (T&) *args[i][1],                   (T&) *args[i][0]); break;
		case SIN:    ((V&) algo).sin_fwd  ((ExprSin&)      *(nodes[i]), (T&) *args[i][1],                   (T&) *args[i][0]); break;
		case TAN:    ((V&) algo).tan_fwd  ((ExprTan&)      *(nodes[i]), (T&) *args[i][1],                   (T&) *args[i][0]); break;
		case COSH:   ((V&) algo).cosh_fwd ((ExprCosh&)     *(nodes[i]), (T&) *args[i][1],                   (T&) *args[i][0]); break;
		case SINH:   ((V&) algo).sinh_fwd ((ExprSinh&)     *(nodes[i]), (T&) *args[i][1],                   (T&) *args[i][0]); break;
		case TANH:   ((V&) algo).tanh_fwd ((ExprTanh&)     *(nodes[i]), (T&) *args[i][1],                   (T&) *args[i][0]); break;
		case ACOS:   ((V&) algo).acos_fwd ((ExprAcos&)     *(nodes[i]), (T&) *args[i][1],                   (T&) *args[i][0]); break;
		case ASIN:   ((V&) algo).asin_fwd ((ExprAsin&)     *(nodes[i]), (T&) *args[i][1],                   (T&) *args[i][0]); break;
		case ATAN:   ((V&) algo).atan_fwd ((ExprAtan&)     *(nodes[i]), (T&) *args[i][1],                   (T&) *args[i][0]); break;
		case ACOSH:  ((V&) algo).acosh_fwd((ExprAcosh&)    *(nodes[i]), (T&) *args[i][1],                   (T&) *args[i][0]); break;
		case ASINH:  ((V&) algo).asinh_fwd((ExprAsinh&)    *(nodes[i]), (T&) *args[i][1],                   (T&) *args[i][0]); break;
		case ATANH:  ((V&) algo).atanh_fwd((ExprAtanh&)    *(nodes[i]), (T&) *args[i][1],                   (T&) *args[i][0]); break;
		}
	}
	return (T&) *expr.deco;
}

template<typename T>template<class V>
void CompiledFunction<T>::backward(const V& algo) const {

	assert(dynamic_cast<const BwdAlgorithm<T>* >(&algo)!=NULL);

	for (int i=0; i<expr.size; i++) {
		switch(code[i]) {
		case IDX:    ((V&) algo).index_bwd((ExprIndex&)    *(nodes[i]), (T&) *args[i][1],                      (T&) *args[i][0]); break;
		case VEC:    ((V&) algo).vector_bwd((ExprVector&)  *(nodes[i]), (T**) &(*args[i][1]),                  (T&) *args[i][0]); break;
		case SYM:    ((V&) algo).symbol_bwd((ExprSymbol&)  *(nodes[i]),                                        (T&) *args[i][0]); break;
		case CST:    ((V&) algo).cst_bwd  ((ExprConstant&) *(nodes[i]),                                        (T&) *args[i][0]); break;
		case APPLY:  ((V&) algo).apply_bwd  ((ExprApply&)  *(nodes[i]), (T**) &(*args[i][1]),                  (T&) *args[i][0]); break;
		case ADD:    ((V&) algo).add_bwd    ((ExprAdd&)    *(nodes[i]), (T&) *args[i][1], (T&) *args[i][2], (T&) *args[i][0]); break;
		case ADD_V:  ((V&) algo).add_V_bwd  ((ExprAdd&)    *(nodes[i]), (T&) *args[i][1], (T&) *args[i][2], (T&) *args[i][0]); break;
		case ADD_M:  ((V&) algo).add_M_bwd  ((ExprAdd&)    *(nodes[i]), (T&) *args[i][1], (T&) *args[i][2], (T&) *args[i][0]); break;
		case MUL:    ((V&) algo).mul_bwd    ((ExprMul&)    *(nodes[i]), (T&) *args[i][1], (T&) *args[i][2], (T&) *args[i][0]); break;
		case MUL_SV: ((V&) algo).mul_SV_bwd ((ExprMul&)    *(nodes[i]), (T&) *args[i][1], (T&) *args[i][2], (T&) *args[i][0]); break;
		case MUL_SM: ((V&) algo).mul_SM_bwd ((ExprMul&)    *(nodes[i]), (T&) *args[i][1], (T&) *args[i][2], (T&) *args[i][0]); break;
		case MUL_VV: ((V&) algo).mul_VV_bwd ((ExprMul&)    *(nodes[i]), (T&) *args[i][1], (T&) *args[i][2], (T&) *args[i][0]); break;
		case MUL_MV: ((V&) algo).mul_MV_bwd ((ExprMul&)    *(nodes[i]), (T&) *args[i][1], (T&) *args[i][2], (T&) *args[i][0]); break;
		case MUL_MM: ((V&) algo).mul_MM_bwd ((ExprMul&)    *(nodes[i]), (T&) *args[i][1], (T&) *args[i][2], (T&) *args[i][0]); break;
		case SUB:    ((V&) algo).sub_bwd    ((ExprSub&)    *(nodes[i]), (T&) *args[i][1], (T&) *args[i][2], (T&) *args[i][0]); break;
		case SUB_V:  ((V&) algo).sub_V_bwd  ((ExprSub&)    *(nodes[i]), (T&) *args[i][1], (T&) *args[i][2], (T&) *args[i][0]); break;
		case SUB_M:  ((V&) algo).sub_M_bwd  ((ExprSub&)    *(nodes[i]), (T&) *args[i][1], (T&) *args[i][2], (T&) *args[i][0]); break;
		case DIV:    ((V&) algo).div_bwd  ((ExprDiv&)      *(nodes[i]), (T&) *args[i][1], (T&) *args[i][2], (T&) *args[i][0]); break;
		case MAX:    ((V&) algo).max_bwd  ((ExprMax&)      *(nodes[i]), (T&) *args[i][1], (T&) *args[i][2], (T&) *args[i][0]); break;
		case MIN:    ((V&) algo).min_bwd  ((ExprMin&)      *(nodes[i]), (T&) *args[i][1], (T&) *args[i][2], (T&) *args[i][0]); break;
		case ATAN2:  ((V&) algo).atan2_bwd((ExprAtan2&)    *(nodes[i]), (T&) *args[i][1], (T&) *args[i][2], (T&) *args[i][0]); break;
		case MINUS:  ((V&) algo).minus_bwd((ExprMinus&)    *(nodes[i]), (T&) *args[i][1],                      (T&) *args[i][0]); break;
		case SIGN:   ((V&) algo).sign_bwd ((ExprSign&)     *(nodes[i]), (T&) *args[i][1],                      (T&) *args[i][0]); break;
		case ABS:    ((V&) algo).abs_bwd  ((ExprAbs&)      *(nodes[i]), (T&) *args[i][1],                      (T&) *args[i][0]); break;
		case POWER:  ((V&) algo).power_bwd((ExprPower&)    *(nodes[i]), (T&) *args[i][1],                      (T&) *args[i][0]); break;
		case SQR:    ((V&) algo).sqr_bwd  ((ExprSqr&)      *(nodes[i]), (T&) *args[i][1],                      (T&) *args[i][0]); break;
		case SQRT:   ((V&) algo).sqrt_bwd ((ExprSqrt&)     *(nodes[i]), (T&) *args[i][1],                      (T&) *args[i][0]); break;
		case EXP:    ((V&) algo).exp_bwd  ((ExprExp&)      *(nodes[i]), (T&) *args[i][1],                      (T&) *args[i][0]); break;
		case LOG:    ((V&) algo).log_bwd  ((ExprLog&)      *(nodes[i]), (T&) *args[i][1],                      (T&) *args[i][0]); break;
		case COS:    ((V&) algo).cos_bwd  ((ExprCos&)      *(nodes[i]), (T&) *args[i][1],                      (T&) *args[i][0]); break;
		case SIN:    ((V&) algo).sin_bwd  ((ExprSin&)      *(nodes[i]), (T&) *args[i][1],                      (T&) *args[i][0]); break;
		case TAN:    ((V&) algo).tan_bwd  ((ExprTan&)      *(nodes[i]), (T&) *args[i][1],                      (T&) *args[i][0]); break;
		case COSH:   ((V&) algo).cosh_bwd ((ExprCosh&)     *(nodes[i]), (T&) *args[i][1],                      (T&) *args[i][0]); break;
		case SINH:   ((V&) algo).sinh_bwd ((ExprSinh&)     *(nodes[i]), (T&) *args[i][1],                      (T&) *args[i][0]); break;
		case TANH:   ((V&) algo).tanh_bwd ((ExprTanh&)     *(nodes[i]), (T&) *args[i][1],                      (T&) *args[i][0]); break;
		case ACOS:   ((V&) algo).acos_bwd ((ExprAcos&)     *(nodes[i]), (T&) *args[i][1],                      (T&) *args[i][0]); break;
		case ASIN:   ((V&) algo).asin_bwd ((ExprAsin&)     *(nodes[i]), (T&) *args[i][1],                      (T&) *args[i][0]); break;
		case ATAN:   ((V&) algo).atan_bwd ((ExprAtan&)     *(nodes[i]), (T&) *args[i][1],                      (T&) *args[i][0]); break;
		case ACOSH:  ((V&) algo).acosh_bwd((ExprAcosh&)    *(nodes[i]), (T&) *args[i][1],                      (T&) *args[i][0]); break;
		case ASINH:  ((V&) algo).asinh_bwd((ExprAsinh&)    *(nodes[i]), (T&) *args[i][1],                      (T&) *args[i][0]); break;
		case ATANH:  ((V&) algo).atanh_bwd((ExprAtanh&)    *(nodes[i]), (T&) *args[i][1],                      (T&) *args[i][0]); break;
		}
	}
}

// for debug only
template<typename T>
const char* CompiledFunction<T>::op(operation o) const {
	switch (o) {
	case IDX:   return "[]";
	case VEC:   return "V";
	case CST:   return "const";
	case SYM:   return "symbl";
	case APPLY: return "apply";
	case ADD: case ADD_V: case ADD_M:
		        return "+";
	case MUL: case MUL_SV: case MUL_SM: case MUL_VV: case MUL_MV: case MUL_MM:
		        return "*";
	case MINUS: case SUB: case SUB_V: case SUB_M:
		        return "-";
	case DIV:   return "/";
	case MAX:   return "max";
	case MIN:   return "min";
	case ATAN2: return "atan2";
	case SIGN:  return "sign";
	case ABS:   return "abs";
	case POWER: return "pow";
	case SQR:   return "sqr";
	case SQRT:  return "sqrt";
	case EXP:   return "exp";
	case LOG:   return "log";
	case COS:   return "cos";
	case  SIN:  return "sin";
	case  TAN:  return "tan";
	case  ACOS: return "acos";
	case  ASIN: return "asin";
	case  ATAN: return "atan";
	case COSH:  return "cosh";
	case SINH:  return "sinh";
	case TANH:  return "tanh";
	case ACOSH: return "acosh";
	case ASINH: return "asinh";
	case ATANH: return "atanh";
	}
}

// for debug only
template<typename T>
std::ostream& operator<<(std::ostream& os,const CompiledFunction<T>& f) {
	for (int i=0; i<f.expr.size; i++) {
		switch(f.code[i]) {
		case CompiledFunction<T>::IDX:
		{
			ExprIndex& e=(ExprIndex&) *(f.nodes[i]);
			os << e.id << ": [-]" << " " << (T&) *f.args[i][0] << " " << e.expr.id << " " << (T&) *f.args[i][1];
		}
		break;
		case CompiledFunction<T>::VEC:
		{
			ExprVector& e=(ExprVector&) *(f.nodes[i]);
			const T** _args=(const T**) &f.args[i][1];
			os << e.id << ": vec " << " ";
			for (int i=0; i<e.nb_args; i++)
				os << (e.arg(i).id) << " " << *(_args[i]) << " ";
		}
		break;
		case CompiledFunction<T>::SYM:
		{
			ExprSymbol& e=(ExprSymbol&) *(f.nodes[i]);
			os << e.id << ": " << e.name << " " << (T&) *f.args[i][0];
		}
		break;
		case CompiledFunction<T>::CST:
		{
			ExprConstant& e=(ExprConstant&) *(f.nodes[i]);
			os << e.id << ": cst=" << e.get_matrix_value() << " " <<  (T&) *f.args[i][0];
		}
		break;
		case CompiledFunction<T>::APPLY:
		{
			ExprApply& e=(ExprApply&) *(f.nodes[i]);
			const T** args=(const T**) f.args[i][1];
			os << e.id << ": " << e.func.name << "()" << " ";
			for (int i=0; i<e.nb_args; i++)
				os << e.arg(i).id << " " << *args[i] << " ";
		}
		break;
		case CompiledFunction<T>::ADD:
		case CompiledFunction<T>::ADD_V:
		case CompiledFunction<T>::ADD_M:
		case CompiledFunction<T>::MUL:
		case CompiledFunction<T>::MUL_SV:
		case CompiledFunction<T>::MUL_SM:
		case CompiledFunction<T>::MUL_VV:
		case CompiledFunction<T>::MUL_MV:
		case CompiledFunction<T>::MUL_MM:
		case CompiledFunction<T>::SUB:
		case CompiledFunction<T>::SUB_V:
		case CompiledFunction<T>::SUB_M:
		case CompiledFunction<T>::DIV:
		case CompiledFunction<T>::MAX:
		case CompiledFunction<T>::MIN:
		case CompiledFunction<T>::ATAN2:
		{
			ExprBinaryOp& e=(ExprBinaryOp&) *(f.nodes[i]);
			os << e.id << ": " << f.op(f.code[i]) << " " << (T&) *f.args[i][0] << " ";
			os << e.left.id << " " << (T&) *f.args[i][1] << " ";
			os << e.right.id << " " << (T&) *f.args[i][2];
		}
		break;

		case CompiledFunction<T>::MINUS:
		case CompiledFunction<T>::SIGN:
		case CompiledFunction<T>::ABS:
		case CompiledFunction<T>::POWER:
		case CompiledFunction<T>::SQR:
		case CompiledFunction<T>::SQRT:
		case CompiledFunction<T>::EXP:
		case CompiledFunction<T>::LOG:
		case CompiledFunction<T>::COS:
		case CompiledFunction<T>::SIN:
		case CompiledFunction<T>::TAN:
		case CompiledFunction<T>::COSH:
		case CompiledFunction<T>::SINH:
		case CompiledFunction<T>::TANH:
		case CompiledFunction<T>::ACOS:
		case CompiledFunction<T>::ASIN:
		case CompiledFunction<T>::ATAN:
		case CompiledFunction<T>::ACOSH:
		case CompiledFunction<T>::ASINH:
		case CompiledFunction<T>::ATANH:
		{
			ExprUnaryOp& e=(ExprUnaryOp&) *(f.nodes[i]);
			os << e.id << ": " << f.op(f.code[i]) << " " << (T&) *f.args[i][0] << " ";
			os << e.expr.id << " " << (T&) *f.args[i][1];
		}
		break;
		}
		os << endl;
	}
	return os;
}

} // namespace ibex
#endif // _IBEX_COMPILED_FUNCTION_H_
