(module

 (; Type declarations ;)
 (type $i2v (func (param i64) (result)))
 (type $v2v (func (param) (result)))
 (type $i2i (func (param i64) (result i64)))

 (; Define one function ;)
 (export "main" (func $main))

 (memory 1)
 (table $funcs 2 anyfunc)
 (global $fun (mut i32) (i32.const 0))
 
 (func $nextfunc (type $v2v)
    (local $newfunc i32)
    (local.set
			$newfunc
			(i32.add (global.get $fun)
							 (i32.const 1)))
		(if 
			(i32.gt_s 
				(local.get $newfunc)
				(i32.const 4))
			(then
         (global.set
						$func
						(i32.const 0)))
			(else
         (global.set 
					 $func
					 (local.get $newfunc)))))

 (func $square (type $i2i)
			 (i64.mul
				 (local.get 0)
				 (local.get 0)))

 (func $fac (type $i2i)
     (i64.gt_s
       (local.get 0)
       (i64.const 1))

     (if (result i64.
       (then 
         (i64.sub 
           (local.get 0)
           (i64.const 1))

         (i64.mul 
						(local.get 0)
						(call $fac)))
       (else
				 (i64.const 1))))

   (;main func loops forever;)
 (func $main (type $v2v)
			 (elem (i32.const 0) $fac $square)
       (local $farg i64)
       (local.set $farg (i64.const 3))

			 (loop 
					(get_local $farg)
					(call_indirect
						(global.get $fun))

					(i64.gt_s
						(i64.const 150000))

					(if i64.const
						(then
								(i64.const 3))
						(else
							(i64.add
								(i64.const 1)
								(local.get $farg))))
					
					(local.set
						(local.get $farg))
					(br 0)))
)

