#ifndef AcumioServer_multi_description_mutations_h
#define AcumioServer_multi_description_mutations_h
//============================================================================
// Name        : multi_description_mutations.h
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : Provides classes for describing requested mutations to a
//               MultiDescriptionHistory. This is similar to the
//               acumio::mem_repository::ElementMutatorInterface and its
//               sub-classes, but all operations are only on
//               MultiDescriptionHistory elements.
//
//               The usual client entry point for making instances of these
//               classes should come through the MultiMutationFactory.
//               The typical usage pattern should be:
//                   MultiMutationFactory my_factory;
//                   std::unique_ptr<MultiMutationInterface> mutation =
//                       my_factory.build(my_mutation_chain);
//               where my_mutation_chain is an instance of a
//               model::MultiDescriptionMutationChain.
//============================================================================

#include <grpc++/support/status.h>
#include "description.pb.h"
#include "multi_mutation_context.h"

namespace acumio {

// Technically, this could be just a typedef of
// mem_repository::ElementMutatorInterface<model::MultiDescriptionHistory>.
// However, our current usage does not include a MultiDescriptionHistory
// being stored on its own; it's always stored with a kind of entity that
// is being described. If that story changes, we can change this, but
// for now, we want to avoid the potential confusion and describe this
// base class differently.
// The point of this class is to modify a MultiDescrptionHistory according
// to the specific rules of the derived implementation classes. Each 
// implementation class has its own rules. The impl classes are hidden
// in the .cpp file; there is no need to expose them as public.
class MultiMutationInterface {
 public:
  MultiMutationInterface();
  virtual ~MultiMutationInterface();
  virtual grpc::Status Mutate(
      model::MultiDescriptionHistory* element) const = 0;
};

class MultiMutationFactory {
 public:
  MultiMutationFactory() {}
  ~MultiMutationFactory() {}
  // Creates a MultiMutationInterface from the provided chain. The
  // returned MultiMutationInterface will perform the operations
  // described in the chain iterating from the first element to
  // the last, performing the operations in the order presented.
  // If there is an error on the kth element, an error will be
  // returned, but unless the failure is on the first change, the
  // errors will not be rolled back. It is the responsibility of
  // the clients to provide rollback capabilities if this is needed,
  // or more precisely, the client should present consistent changes
  // at the outset.
  std::unique_ptr<MultiMutationInterface> build(
      const model::MultiDescriptionMutationChain& chain) const;
};

} // namespace acumio

#endif // AcumioServer_multi_description_mutations_h
