# frozen_string_literal: true
# typed: true

# Use as a mixin with extend (`extend T::Generic`).
# Docs at https://hackpad.corp.stripe.com/Type-Validation-in-pay-server-1JaoTHir5Mo.
module T::Generic
  include T::Helpers
  include Kernel

  ### Class/Module Helpers ###

  def [](*types)
    self
  end

  def type_member(variance=:invariant, fixed: nil, lower: T.untyped, upper: BasicObject)
    T::Types::TypeMember.new(variance) # rubocop:disable PrisonGuard/UseOpusTypesShortcut
  end

  def type_template(variance=:invariant, fixed: nil, lower: T.untyped, upper: BasicObject)
    T::Types::TypeTemplate.new(variance) # rubocop:disable PrisonGuard/UseOpusTypesShortcut
  end

  def experimental_attached_class_new(*args, &blk)
    T.unsafe(self).send(:new, *args, &blk)
  end
end
