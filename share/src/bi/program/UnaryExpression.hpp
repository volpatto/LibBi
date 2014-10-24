/**
 * @file
 *
 * @author Lawrence Murray <lawrence.murray@csiro.au>
 * $Rev$
 * $Date$
 */
#ifndef BI_PROGRAM_UNARYEXPRESSION_HPP
#define BI_PROGRAM_UNARYEXPRESSION_HPP

#include "Expression.hpp"
#include "Operator.hpp"

namespace biprog {
/**
 * Unary expression.
 *
 * @ingroup program
 */
class UnaryExpression: public Expression {
public:
  /**
   * Constructor.
   *
   * @param op Operator.
   * @param right Right operand.
   */
  UnaryExpression(Operator* op, Expression* right);

  /**
   * Destructor.
   */
  virtual ~UnaryExpression();

  /**
   * Operator.
   */
  boost::scoped_ptr<Operator> op;

  /**
   * Right operand.
   */
  boost::scoped_ptr<Expression> right;
};
}

inline biprog::UnaryExpression::UnaryExpression(Operator* op,
    Expression* right) :
    op(op), right(right) {
  //
}

inline biprog::UnaryExpression::~UnaryExpression() {
  //
}

#endif

