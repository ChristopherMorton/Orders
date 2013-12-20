#include "orders.h"

namespace sum
{

void Order::initOrder( Order_Action a, Order_Conditional c, int cnt )
{
   action = a;
   condition = c;
   count = cnt;
}

Order::Order( Order_Action a, Order_Conditional c, int cnt )
{
   initOrder( a, c, cnt );
}

Order::Order( Order_Action a, Order_Conditional c )
{
   initOrder( a, c, 1 );
}

Order::Order( Order_Action a, int cnt)
{
   initOrder( a, TRUE, cnt );
}

Order::Order( Order_Action a )
{
   initOrder( a, TRUE, 1 );
}

Order::Order()
{
   initOrder( WAIT, TRUE, 1 ); 
}

/*
Order::Order( Order &right )
{
   initOrder( right.action, right.condition, right.count );
}
*/

}
