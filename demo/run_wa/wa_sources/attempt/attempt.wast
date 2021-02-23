(module
  (type (;0;) (func))
  (type (;1;) (func (param i32) (result i32)))
  (type (;2;) (func (result i32)))
  (type (;3;) (func (param i32)))
  (type (;4;) (func (param i32 i32)))
  (type (;5;) (func (param i32 i32) (result i32)))
  (import "env" "stackSave" (func $stackSave (type 2)))
  (import "env" "memory" (memory (;0;) 0))
  (import "env" "__indirect_function_table" (table (;0;) 0 funcref))
  (func $__wasm_call_ctors (type 0)
    call $__wasm_apply_relocs)
  (func $__wasm_apply_relocs (type 0)
    nop)
  (func $emptyBoard (type 3) (param i32)
    local.get 0
    i64.const 6872316419617283935
    i64.store align=1
    local.get 0
    i32.const 0
    i32.store offset=12
    local.get 0
    i32.const 20319
    i32.store16 offset=8 align=1)
  (func $isFull (type 1) (param i32) (result i32)
    block  ;; label = @1
      local.get 0
      i32.load8_u
      i32.const 95
      i32.eq
      br_if 0 (;@1;)
      local.get 0
      i32.load8_u offset=1
      i32.const 95
      i32.eq
      br_if 0 (;@1;)
      local.get 0
      i32.load8_u offset=2
      i32.const 95
      i32.eq
      br_if 0 (;@1;)
      local.get 0
      i32.load8_u offset=3
      i32.const 95
      i32.eq
      br_if 0 (;@1;)
      local.get 0
      i32.load8_u offset=4
      i32.const 95
      i32.eq
      br_if 0 (;@1;)
      local.get 0
      i32.load8_u offset=5
      i32.const 95
      i32.eq
      br_if 0 (;@1;)
      local.get 0
      i32.load8_u offset=6
      i32.const 95
      i32.eq
      br_if 0 (;@1;)
      local.get 0
      i32.load8_u offset=7
      i32.const 95
      i32.eq
      br_if 0 (;@1;)
      local.get 0
      i32.load8_u offset=8
      i32.const 95
      i32.ne
      return
    end
    i32.const 0)
  (func $nextPlayer (type 1) (param i32) (result i32)
    i32.const 88
    i32.const 79
    local.get 0
    i32.const 79
    i32.eq
    select)
  (func $play (type 4) (param i32 i32)
    local.get 0
    local.get 0
    i32.load8_s offset=9
    local.get 1
    call_indirect (type 1)
    local.tee 1
    i32.store8 offset=9
    local.get 0
    local.get 0
    i32.load offset=12
    i32.add
    local.get 1
    i32.store8
    local.get 0
    local.get 0
    i32.load offset=12
    i32.const 1
    i32.add
    i32.store offset=12)
  (func $__original_main (type 2) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32)
    call $stackSave
    i32.const 16
    i32.sub
    local.tee 0
    local.tee 11
    i32.const 8
    i32.add
    i32.const 20319
    i32.store16
    local.get 0
    i64.const 6872316419617283935
    i64.store
    local.get 0
    i32.const 0
    i32.store offset=12
    i32.const 95
    local.set 3
    i32.const 95
    local.set 4
    i32.const 95
    local.set 5
    i32.const 95
    local.set 6
    i32.const 95
    local.set 7
    i32.const 95
    local.set 8
    i32.const 95
    local.set 9
    i32.const 95
    local.set 10
    i32.const 95
    local.set 1
    loop  ;; label = @1
      local.get 0
      block (result i32)  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            local.get 1
            i32.const 95
            i32.eq
            br_if 0 (;@4;)
            local.get 10
            i32.const 95
            i32.eq
            br_if 0 (;@4;)
            local.get 9
            i32.const 95
            i32.eq
            br_if 0 (;@4;)
            local.get 8
            i32.const 95
            i32.eq
            br_if 0 (;@4;)
            local.get 7
            i32.const 95
            i32.eq
            br_if 0 (;@4;)
            local.get 6
            i32.const 95
            i32.eq
            br_if 0 (;@4;)
            local.get 5
            i32.const 95
            i32.eq
            br_if 0 (;@4;)
            local.get 4
            i32.const 95
            i32.eq
            br_if 0 (;@4;)
            local.get 3
            i32.const 95
            i32.eq
            br_if 0 (;@4;)
            local.get 11
            i32.const 20319
            i32.store16 offset=8
            local.get 0
            i64.const 6872316419617283935
            i64.store
            i32.const 0
            local.set 2
            local.get 0
            i32.const 0
            i32.store offset=12
            br 1 (;@3;)
          end
          i32.const 79
          local.tee 1
          local.get 0
          i32.load8_u offset=9
          i32.const 79
          i32.ne
          br_if 1 (;@2;)
          drop
        end
        i32.const 88
      end
      local.tee 1
      i32.store8 offset=9
      local.get 0
      local.get 2
      i32.add
      local.get 1
      i32.store8
      local.get 0
      local.get 0
      i32.load offset=12
      i32.const 1
      i32.add
      local.tee 2
      i32.store offset=12
      local.get 0
      i32.load8_u offset=8
      local.set 3
      local.get 0
      i32.load8_u offset=7
      local.set 4
      local.get 0
      i32.load8_u offset=6
      local.set 5
      local.get 0
      i32.load8_u offset=5
      local.set 6
      local.get 0
      i32.load8_u offset=4
      local.set 7
      local.get 0
      i32.load8_u offset=3
      local.set 8
      local.get 0
      i32.load8_u offset=2
      local.set 9
      local.get 0
      i32.load8_u offset=1
      local.set 10
      local.get 0
      i32.load8_u
      local.set 1
      br 0 (;@1;)
    end
    unreachable)
  (func $main (type 5) (param i32 i32) (result i32)
    call $__original_main)
  (func $__post_instantiate (type 0)
    call $__wasm_call_ctors)
  (global (;0;) i32 (i32.const 0))
  (export "__wasm_apply_relocs" (func $__wasm_apply_relocs))
  (export "emptyBoard" (func $emptyBoard))
  (export "isFull" (func $isFull))
  (export "nextPlayer" (func $nextPlayer))
  (export "play" (func $play))
  (export "__original_main" (func $__original_main))
  (export "main" (func $main))
  (export "__dso_handle" (global 0))
  (export "__post_instantiate" (func $__post_instantiate)))
