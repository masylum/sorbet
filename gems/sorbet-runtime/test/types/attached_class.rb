# frozen_string_literal: true
require_relative '../test_helper'

module Opus::Types::Test
  class AttachedClassTest < Critic::Unit::UnitTest
    describe "T.experimental_attached_class" do
      it 'can type self methods' do

        class Base
          extend T::Sig

          sig {returns(T.experimental_attached_class)}
          def self.make
            new
          end
        end

        class A < Base; end

        assert_equal(Base.make.class, Base)
        assert_equal(A.make.class, A)
      end

      it 'can type self methods that use self.new' do

        class Base
          extend T::Sig

          sig {returns(T.experimental_attached_class)}
          def self.make
            self.new
          end
        end

        assert_equal(Base.make.class, Base)
      end

      it 'does not throw when the returned value is bad' do

        class Base
          extend T::Sig

          sig {returns(T.experimental_attached_class)}
          def self.make
            10
          end
        end

        assert_equal(Base.make.class, Integer)
      end
    end

    describe "T.experimental_attached_class_new" do
      it 'works with constructors that have no args' do

        class NoArgs
          extend T::Sig
          extend T::Generic

          def initialize; end
        end

        assert_equal(NoArgs.experimental_attached_class_new.class, NoArgs)
      end

      it 'works with constructors that have positional args' do
        class PosArgs
          extend T::Sig
          extend T::Generic

          def initialize(a, b); end
        end

        assert_equal(PosArgs.experimental_attached_class_new(1, 2).class, PosArgs)
        assert_raises(ArgumentError) { PosArgs.experimental_attached_class_new }
      end

      it 'works with constructors that have default args' do
        class DefArgs
          extend T::Sig
          extend T::Generic

          def initialize(a, b=10); end
        end

        assert_equal(DefArgs.experimental_attached_class_new(1).class, DefArgs)
        assert_equal(DefArgs.experimental_attached_class_new(1, 20).class, DefArgs)
        assert_raises(ArgumentError) { DefArgs.experimental_attached_class_new }
      end

      it 'works with constructors that take varargs' do
        class VarArgs
          extend T::Sig
          extend T::Generic

          def initialize(a, *args); end
        end

        assert_equal(VarArgs.experimental_attached_class_new(1).class, VarArgs)
        assert_equal(VarArgs.experimental_attached_class_new(1, 2).class, VarArgs)
        assert_raises(ArgumentError) { VarArgs.experimental_attached_class_new }
      end

      it 'works with constructors that take keyword args' do
        class KwArgs
          extend T::Sig
          extend T::Generic

          def initialize(x:, y: 3); end
        end

        assert_equal(KwArgs.experimental_attached_class_new(x: 1, y: 2).class, KwArgs)
        assert_equal(KwArgs.experimental_attached_class_new(x: 1, y: 2).class, KwArgs)
        assert_raises(ArgumentError) { KwArgs.experimental_attached_class_new }
      end

      it 'works with constructors that take blocks' do
        class BlockArgs
          extend T::Sig
          extend T::Generic

          def initialize(&blk); blk.call; end
        end

        assert_equal(BlockArgs.experimental_attached_class_new{}.class, BlockArgs)

        # `.call` doesn't exist on `nil`
        assert_raises(NoMethodError) { BlockArgs.experimental_attached_class_new }
      end
    end
  end
end
