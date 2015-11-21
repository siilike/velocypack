#include <iostream>
#include "velocypack/vpack.h"

using namespace arangodb::velocypack;

int main (int, char*[]) {
  // create an object with a few members
  Builder b;

  b(Value(ValueType::Object));
  b.add("foo", Value(42)); 
  b.add("bar", Value("some string value")); 
  b.add("baz", Value(ValueType::Object));
  b.add("qux", Value(true));
  b.add("bart", Value("this is a string"));
  b.close();
  b.add("quux", Value(ValueType::Array));
  b.add(Value(1));
  b.add(Value(2));
  b.add(Value(3));
  b.close();
  b.close();

  // a Slice is a lightweight accessor for a VPack value
  Slice s(b.start());

  // get all object keys. returns a vector of strings
  for (auto& it : Collection::keys(s)) {
    // print key
    std::cout << "Object has key '" << it << "'" << std::endl;
  }

  // get all object values. returns a Builder object with an Array inside
  Builder values = Collection::values(s);
  for (auto it : ArrayIterator(values.slice())) {
    std::cout << "Object value is: " << it << ", as JSON: " << it.toJson() << std::endl;
  }

  // recursively visit all members in the Object
  // PostOrder here means we'll be visiting compound members before
  // we're diving into their subvalues
  Collection::visitRecursive(s, Collection::PostOrder, [] (Slice const& key, Slice const& value) -> bool {
    if (! key.isNone()) {
      // we are visiting an Object member
      std::cout << "Visiting Object member: " << key.copyString() << ", value: " << value.toJson() << std::endl;
    }
    else {
      // we are visiting an Array member
      std::cout << "Visiting Array member: " << value.toJson() << std::endl;
    }
    // to continue visiting, return true. to abort visiting, return false
    return true; 
  });
}