#ifndef UDHO_SESSION_SESSION_H
#define UDHO_SESSION_SESSION_H

#include <udho/session/fwd.h>
#include <udho/session/record.h>
#include <udho/session/note.h>
#include <udho/session/catalogue.h>

namespace udho {

/**
 * Each session is identified by a sessid. Each session has a `record` which is a map, mapping string key and values
 * refered to as attributes. These records are stored using a storage service such as disk, key value store, DBMS etc.
 * An HTTP request may be associated with a sessid. Once a request associated with a sessid is received, the server
 * loads the attributes of that session from the storage. If the storage doesn't contain record for that sessid then
 * the server create and empty record and then load it. An action handling that request may read or write the `record`
 * for the session.
 *
 * Consistency
 * ===========
 *
 * The modified `record` is stored using the storage service after it has been modified and no longer being read by
 * any other actions. If the record is not mofified by an action then it is not stored, however other actions are
 * notified that it is no longer being read by that action.
 *
 * Given two actios a1, a2 handling two HTTP requests associated with the same sessid, while a2 starting after a1 has
 * ended, and the record is modified by a1, it is garunteed that a2 reads the modified record.
 *
 * If a2 starts after a1 has started and before a1 has ended then they both can read or write the same record. However,
 * the record is not written to the storage until all actions associated with the same sessid (a1, a2 in this example)
 * finishes processing. Unless an attribute is specifically guarded while modifying, whichever finishes latter determines
 * the latest state of the record. In this example, if a1 finishes after a2 and they both modify same property p as p1,
 * and p2, then the action a3 associated with the same sessid, which starts after both a1 and a2 ends, gets the value of
 * p as p1, even though a2 started after a1. The record is saved to the storage after both a1 and a2 finishes.
 *
 * However, if a2 performs set operation on key p as set(p, "p2", NOT_OVERWRITABLE) then a1 performing another set operation
 * on p will throw exception. [Feature deffered]
 *
 * Additionally, it garuntees that reading or writting the attributes of the record is a thread safe operation.
 */
namespace session {

}

}


#endif // UDHO_SESSION_SESSION_H
