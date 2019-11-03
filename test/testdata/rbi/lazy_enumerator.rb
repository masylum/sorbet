# typed: true

T.assert_type!([1].lazy.collect(&:to_s), Enumerator::Lazy[String])
T.assert_type!([1].lazy.collect, Enumerator::Lazy[String])
T.assert_type!(['abcd'].lazy.collect_concat{|s| s.chars.map(&:ord) }, Enumerator::Lazy[Integer])
T.assert_type!([1].lazy.drop(4), Enumerator::Lazy[Integer])
T.assert_type!([1].lazy.drop_while(&:even?), Enumerator::Lazy[Integer])
T.assert_type!([1].lazy.flat_map{|i| [i.to_s]}, Enumerator::Lazy[String])
T.assert_type!([1].lazy.grep(1..10), Enumerator::Lazy[Integer])
T.assert_type!([1].lazy.map(&:to_s), Enumerator::Lazy[String])
T.assert_type!([1].lazy.reject(&:even?), Enumerator::Lazy[Integer])
T.assert_type!([1].lazy.select(&:even?), Enumerator::Lazy[Integer])
T.assert_type!([1].lazy.take(10), Enumerator::Lazy[Integer])
T.assert_type!([1].lazy.take_while{|i| i < 10 }, Enumerator::Lazy[Integer])
