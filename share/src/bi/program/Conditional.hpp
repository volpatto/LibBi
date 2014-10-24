/**
 * @file
 *
 * @author Lawrence Murray <lawrence.murray@csiro.au>
 * $Rev$
 * $Date$
 */
#ifndef BI_PROGRAM_CONDITIONAL_HPP
#define BI_PROGRAM_CONDITIONAL_HPP

#include "Statement.hpp"
#include "Expression.hpp"

#include "boost/scoped_ptr.hpp"

namespace biprog {
/**
 * Conditional.
 *
 * @ingroup program
 */
class Conditional: public Statement {
public:
  /**
   * Constructor.
   */
  Conditional(Expression* cond, Expression* body = NULL);

  /**
   * Destructor.
   */
  virtual ~Conditional();

  /**
   * Condition.
   */
  boost::scoped_ptr<Expression> cond;

  /**
   * Body.
   */
  boost::scoped_ptr<Expression> body;
};
}

inline biprog::Conditional::Conditional(Expression* cond, Expression* body) :
    cond(cond), body(body) {
  //
}

inline biprog::Conditional::~Conditional() {
  //
}

#endif
