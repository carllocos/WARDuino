(module

 (; Type declarations ;)
 (type $any (func))
 (type $i2v (func (param i64) (result)))
 (type $v2v (func (param) (result)))
 (type $i2i (func (param i64) (result i64)))

 (; Define one function ;)
 (export "main" (func $main))

 (memory 1)
 (table funcref
				(elem $fac $square))
 (global $func (mut i32) (i32.const 0))
 
 (func $nextfunc (type $v2v)
    (local $newfunc i32)
    (local.set
			$newfunc
			(i32.add (global.get $func)
							 (i32.const 1)))
		(i32.gt_s 
			(local.get $newfunc)
			(i32.const 1))

		(if 
			(result i32)
			(then
				(i32.const 0))
			(else
					 (local.get $newfunc)))	

    (global.set $func))

 (func $square (type $i2i) (param $p1 i64) (result i64)
			 (i64.mul
				 (local.get $p1)
				 (local.get $p1)))

 (func $fac (type $i2i) (param $p1 i64) (result i64)
		 (i64.gt_s
			 (local.get $p1)
			 (i64.const 1))

		 (if (result i64)
			 (then 
				 (i64.sub 
					 (local.get $p1)
					 (i64.const 1))
				 (call $fac)
				 (local.get $p1)
				 i64.mul)
			 (else
				 (i64.const 1))))

   (;main func loops forever;)
 (func $main (type $v2v)
       (local $farg i64)
       (local.set $farg (i64.const 3))

			 (loop 
					(; (get_local $farg) ;)
					(call_indirect
						(type $i2i)
						(get_local $farg)
						(global.get $func))

				  (i64.const 1500)
					i64.gt_s

					(if (result i64)
							(then
								(i64.const 3))
							(else
								(i64.add
										(i64.const 1)
										(local.get $farg))))
					
					(local.set $farg)
					(call $nextfunc)
					(br 0)))
)

