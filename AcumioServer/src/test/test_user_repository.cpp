//============================================================================
// Name        : test_user_repository.cpp
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : A test_driver for the UserRepository class.
//============================================================================
#include "user_repository.h"

#include "encrypter.h"
#include "test_macros.h"
#include "user.pb.h"

namespace acumio {
namespace {
void PopulateBasicUser(const std::string& name,
                       model::User_AcumioRole role,
                       const std::string& email,
                       const std::string& acumio_login_identity,
                       model::User* user) {
  user->Clear();
  user->set_name(name);
  user->set_role(role);
  user->set_contact_email(email);
  model::Subject* identity = user->mutable_identity();
  model::Principal* primary_identity = identity->mutable_primary_identity();
  primary_identity->set_type(model::Principal_Type_ACUMIO);
  primary_identity->set_name(acumio_login_identity);
}

void SetToUserAlok(model::User* user_alok) {
  PopulateBasicUser("alok", model::User_AcumioRole_USER, "alok@acumio.com",
                    "alok", user_alok);
}

void SetToUserBill(model::User* user_bill) {
  PopulateBasicUser("bill", model::User_AcumioRole_USER, "bill@acumio.com",
                    "bill", user_bill);
}

void SetToUserGajanan(model::User* user_gajanan) {
  PopulateBasicUser("gajanan", model::User_AcumioRole_USER,
                    "gajanan@acumio.com", "gajanan", user_gajanan);
}

// Thanks to C++11, this is now thread-safe. Yay! :)
static crypto::EncrypterInterface* GetEncrypter() {
  static crypto::NoOpEncrypter encrypter;
  return &encrypter;
}

// Thanks to C++11, this is now thread-safe. Yay! :)
static crypto::SaltGeneratorInterface* GetSalter() {
  static crypto::DeterministicSaltGenerator generator(1);
  return &generator;
}

TEST(UserRepository, FullUserConstruction) {
  model::User bill;
  SetToUserBill(&bill);
  std::string salt = (*GetSalter())();
  std::string encrypted = (*GetEncrypter())("really_bad_password", salt);
  // The expectation is that the full_user has a complete new copy
  // of the base-user, so that modifying them does not modify the
  // FullUser.
  FullUser full_user_bill(bill, encrypted, salt);
  bill.set_name("not bill");
  EXPECT_NE(full_user_bill.user().name(), bill.name());
}

TEST(UserRepository, MultiUserConstruction) {
  model::User alok;
  SetToUserAlok(&alok);
  std::string alok_salt = (*GetSalter())();
  std::string alok_pw = (*GetEncrypter())("alok_pw", alok_salt);
  FullUser full_alok(alok, alok_pw, alok_salt);
  model::User bill;
  SetToUserBill(&bill);
  std::string bill_salt = (*GetSalter())();
  std::string bill_pw = (*GetEncrypter())("bill_pw", bill_salt);
  FullUser full_bill(bill, bill_pw, bill_salt);
  model::User gajanan;
  SetToUserGajanan(&gajanan);
  std::string gajanan_salt = (*GetSalter())();
  std::string gajanan_pw = (*GetEncrypter())("gajanan_pw", gajanan_salt);
  FullUser full_gajanan(gajanan, gajanan_pw, gajanan_salt);
}

TEST(UserRepository, TestRepositoryConstruction) {
  UserRepository repository;
  EXPECT_EQ(0, 0);
}

TEST(UserRepository, AddThenRetrieve) {
  model::User alok;
  SetToUserAlok(&alok);
  std::string alok_salt = (*GetSalter())();
  std::string alok_pw = (*GetEncrypter())("alok_pw", alok_salt);
  FullUser full_alok(alok, alok_pw, alok_salt);
  model::User bill;
  SetToUserBill(&bill);
  std::string bill_salt = (*GetSalter())();
  std::string bill_pw = (*GetEncrypter())("bill_pw", bill_salt);
  FullUser full_bill(bill, bill_pw, bill_salt);
  model::User gajanan;
  SetToUserGajanan(&gajanan);
  std::string gajanan_salt = (*GetSalter())();
  std::string gajanan_pw = (*GetEncrypter())("gajanan_pw", gajanan_salt);
  FullUser full_gajanan(gajanan, gajanan_pw, gajanan_salt);

  UserRepository repository;
  grpc::Status check;
  EXPECT_OK(repository.Add(full_alok));
  EXPECT_OK(repository.Add(full_bill));
  EXPECT_OK(repository.Add(full_gajanan));
  FullUser found_alok;
  EXPECT_OK(repository.Get("alok", &found_alok));
  FullUser found_bill;
  EXPECT_OK(repository.Get("bill", &found_bill));
  FullUser found_gajanan;
  EXPECT_OK(repository.Get("gajanan", &found_gajanan));
  EXPECT_EQ(found_alok.user().name(), "alok");
  EXPECT_EQ(found_alok.password(), alok_pw);
  EXPECT_EQ(found_bill.user().name(), "bill");
  EXPECT_EQ(found_bill.password(), bill_pw);
  EXPECT_EQ(found_gajanan.user().name(), "gajanan");
  EXPECT_EQ(found_gajanan.password(), gajanan_pw);
}

TEST(UserRepository, AddDuplicate) {
  model::User bill;
  SetToUserBill(&bill);
  std::string bill_salt = (*GetSalter())();
  std::string bill_pw = (*GetEncrypter())("bill_pw", bill_salt);
  FullUser full_bill(bill, bill_pw, bill_salt);
  FullUser bills_clone(bill, bill_pw, bill_salt);
  UserRepository repository;
  grpc::Status check;
  EXPECT_OK(repository.Add(full_bill));
  EXPECT_ERROR(repository.Add(bills_clone), grpc::StatusCode::ALREADY_EXISTS);
}

TEST(UserRepository, AddRemoveAddGet) {
  model::User alok;
  SetToUserAlok(&alok);
  std::string alok_salt = (*GetSalter())();
  std::string alok_pw = (*GetEncrypter())("alok_pw", alok_salt);
  FullUser full_alok(alok, alok_pw, alok_salt);
  model::User bill;
  SetToUserBill(&bill);
  std::string bill_salt = (*GetSalter())();
  std::string bill_pw = (*GetEncrypter())("bill_pw", bill_salt);
  FullUser full_bill(bill, bill_pw, bill_salt);
  model::User gajanan;
  SetToUserGajanan(&gajanan);
  std::string gajanan_salt = (*GetSalter())();
  std::string gajanan_pw = (*GetEncrypter())("gajanan_pw", gajanan_salt);
  FullUser full_gajanan(gajanan, gajanan_pw, gajanan_salt);
  UserRepository repository;
  grpc::Status check;
  EXPECT_OK(repository.Add(full_alok));
  EXPECT_OK(repository.Add(full_bill));
  EXPECT_OK(repository.Add(full_gajanan));
  EXPECT_OK(repository.Remove("alok"));
  EXPECT_OK(repository.Remove("bill"));
  EXPECT_OK(repository.Remove("gajanan"));
  FullUser found_alok;
  EXPECT_ERROR(repository.Get("alok", &found_alok),
               grpc::StatusCode::NOT_FOUND);
  FullUser found_bill;
  EXPECT_ERROR(repository.Get("bill", &found_bill),
               grpc::StatusCode::NOT_FOUND);
  FullUser found_gajanan;
  EXPECT_ERROR(repository.Get("gajanan", &found_gajanan),
               grpc::StatusCode::NOT_FOUND);
}

TEST(UserRepository, DeleteNotPresent) {
  UserRepository repository;
  EXPECT_ERROR(repository.Remove("bill"), grpc::StatusCode::NOT_FOUND);
  model::User alok;
  SetToUserAlok(&alok);
  std::string alok_salt = (*GetSalter())();
  std::string alok_pw = (*GetEncrypter())("alok_pw", alok_salt);
  FullUser full_alok(alok, alok_pw, alok_salt);
  model::User bill;
  SetToUserBill(&bill);
  std::string bill_salt = (*GetSalter())();
  std::string bill_pw = (*GetEncrypter())("bill_pw", bill_salt);
  FullUser full_bill(bill, bill_pw, bill_salt);
  model::User gajanan;
  SetToUserGajanan(&gajanan);
  std::string gajanan_salt = (*GetSalter())();
  std::string gajanan_pw = (*GetEncrypter())("gajanan_pw", gajanan_salt);
  FullUser full_gajanan(gajanan, gajanan_pw, gajanan_salt);
  grpc::Status check;
  EXPECT_OK(repository.Add(full_alok));
  EXPECT_OK(repository.Add(full_bill));
  EXPECT_OK(repository.Add(full_gajanan));
  // Picking one to try to retrieve remove, sorted before all entries.
  EXPECT_ERROR(repository.Remove("alice"), grpc::StatusCode::NOT_FOUND);
  // Picking one to try to retrieve remove, after all entries.
  EXPECT_ERROR(repository.Remove("zoe"), grpc::StatusCode::NOT_FOUND);
  // Picking one to try to retrieve remove in the middle of the entries.
  EXPECT_ERROR(repository.Remove("bob"), grpc::StatusCode::NOT_FOUND);
}

TEST(UserRepository, UpdateNoKeyChange) {
  model::User bill;
  SetToUserBill(&bill);
  std::string bill_salt = (*GetSalter())();
  std::string bill_pw = (*GetEncrypter())("bill_pw", bill_salt);
  FullUser full_bill(bill, bill_pw, bill_salt);
  UserRepository repository;
  grpc::Status check;
  EXPECT_OK(repository.Add(full_bill));
  (*full_bill.mutable_salt()) = (*GetSalter())();
  (*full_bill.mutable_password()) = (*GetEncrypter())("bill_new_pw",
                                                      full_bill.salt());
  EXPECT_OK(repository.Update("bill", full_bill));
  FullUser found_bill;
  EXPECT_OK(repository.Get("bill", &found_bill));
  EXPECT_EQ(found_bill.password(), full_bill.password());
  EXPECT_EQ(found_bill.salt(), full_bill.salt());
  FullUser sam = full_bill;
  (*sam.mutable_user()->mutable_name()) = "sam";
  EXPECT_ERROR(repository.Update("sam", full_bill),
               grpc::StatusCode::NOT_FOUND);
}

TEST(UserRepository, UpdateWithKeyChange) {
  // We want to test a few things here with respect to key changes:
  // First, Updating first element to a value at the middle then
  // to a value at the end. Both of these should fail with an ALREADY_EXISTS
  // result. Next, move the first value to a position before the first elt,
  // then to a value between first elt and before second elt, then update
  // to a position after the second elt but before last elt and then finally -
  // after restoring it to a first elt position, update it to be a value
  // after last elt. All of these operations should succeed.
  // Next - after restoring first elt back to original position, we do the
  // same with a middle elt: move it to some place before beginning then
  // to another middle location then to some place after end.
  // We finish with the final elt being moved around.
  //
  // After working with existing elements, we will finally test results when
  // we try to update a non-existent element with a key change.
  model::User alok;
  SetToUserAlok(&alok);
  std::string alok_salt = (*GetSalter())();
  std::string alok_pw = (*GetEncrypter())("alok_pw", alok_salt);
  FullUser full_alok(alok, alok_pw, alok_salt);
  model::User bill;
  SetToUserBill(&bill);
  std::string bill_salt = (*GetSalter())();
  std::string bill_pw = (*GetEncrypter())("bill_pw", bill_salt);
  FullUser full_bill(bill, bill_pw, bill_salt);
  model::User gajanan;
  SetToUserGajanan(&gajanan);
  std::string gajanan_salt = (*GetSalter())();
  std::string gajanan_pw = (*GetEncrypter())("gajanan_pw", gajanan_salt);
  FullUser full_gajanan(gajanan, gajanan_pw, gajanan_salt);
  UserRepository repository;

  grpc::Status result;
  EXPECT_OK(repository.Add(full_alok));
  EXPECT_OK(repository.Add(full_bill));
  EXPECT_OK(repository.Add(full_gajanan));
  ASSERT_EQ(repository.size(), 3);
  (*full_alok.mutable_user()->mutable_name()) = "bill";
  EXPECT_ERROR(repository.Update("alok", full_alok),
               grpc::StatusCode::ALREADY_EXISTS);
  ASSERT_EQ(repository.size(), 3);
  (*full_alok.mutable_user()->mutable_name()) = "gajanan";
  EXPECT_ERROR(repository.Update("alok", full_alok),
               grpc::StatusCode::ALREADY_EXISTS);
  ASSERT_EQ(repository.size(), 3);
  (*full_alok.mutable_user()->mutable_name()) = "aa";
  EXPECT_OK(repository.Update("alok", full_alok));
  ASSERT_EQ(repository.size(), 3);
  (*full_alok.mutable_user()->mutable_name()) = "alok";
  EXPECT_OK(repository.Update("aa", full_alok));
  ASSERT_EQ(repository.size(), 3);
  (*full_alok.mutable_user()->mutable_name()) = "aloks";
  EXPECT_OK(repository.Update("alok", full_alok));
  ASSERT_EQ(repository.size(), 3);
  (*full_alok.mutable_user()->mutable_name()) = "bz";
  EXPECT_OK(repository.Update("aloks", full_alok));
  ASSERT_EQ(repository.size(), 3);
  (*full_alok.mutable_user()->mutable_name()) = "alok";
  EXPECT_OK(repository.Update("bz", full_alok));
  ASSERT_EQ(repository.size(), 3);
  (*full_alok.mutable_user()->mutable_name()) = "zz";
  EXPECT_OK(repository.Update("alok", full_alok));
  ASSERT_EQ(repository.size(), 3);
  (*full_alok.mutable_user()->mutable_name()) = "alok";
  EXPECT_OK(repository.Update("zz", full_alok));
  ASSERT_EQ(repository.size(), 3);

  // Done working with first element. Now, moving to middle element.
  (*full_bill.mutable_user()->mutable_name()) = "alok";
  EXPECT_ERROR(repository.Update("bill", full_bill),
               grpc::StatusCode::ALREADY_EXISTS);
  ASSERT_EQ(repository.size(), 3);
  (*full_bill.mutable_user()->mutable_name()) = "gajanan";
  EXPECT_ERROR(repository.Update("bill", full_bill),
               grpc::StatusCode::ALREADY_EXISTS);
  ASSERT_EQ(repository.size(), 3);
  (*full_bill.mutable_user()->mutable_name()) = "aa";
  EXPECT_OK(repository.Update("bill", full_bill));
  ASSERT_EQ(repository.size(), 3);
  (*full_bill.mutable_user()->mutable_name()) = "bill";
  EXPECT_OK(repository.Update("aa", full_bill));
  ASSERT_EQ(repository.size(), 3);
  (*full_bill.mutable_user()->mutable_name()) = "billp";
  EXPECT_OK(repository.Update("bill", full_bill));
  ASSERT_EQ(repository.size(), 3);
  (*full_bill.mutable_user()->mutable_name()) = "zz";
  EXPECT_OK(repository.Update("billp", full_bill));
  ASSERT_EQ(repository.size(), 3);
  (*full_bill.mutable_user()->mutable_name()) = "bill";
  EXPECT_OK(repository.Update("zz", full_bill));
  ASSERT_EQ(repository.size(), 3);

  // Finally, working with last element.
  (*full_gajanan.mutable_user()->mutable_name()) = "alok";
  EXPECT_ERROR(repository.Update("gajanan", full_gajanan),
               grpc::StatusCode::ALREADY_EXISTS);
  ASSERT_EQ(repository.size(), 3);
  (*full_gajanan.mutable_user()->mutable_name()) = "bill";
  EXPECT_ERROR(repository.Update("gajanan", full_gajanan),
               grpc::StatusCode::ALREADY_EXISTS);
  ASSERT_EQ(repository.size(), 3);
  (*full_gajanan.mutable_user()->mutable_name()) = "aa";
  EXPECT_OK(repository.Update("gajanan", full_gajanan));
  ASSERT_EQ(repository.size(), 3);
  (*full_gajanan.mutable_user()->mutable_name()) = "gajanan";
  EXPECT_OK(repository.Update("aa", full_gajanan));
  ASSERT_EQ(repository.size(), 3);
  (*full_gajanan.mutable_user()->mutable_name()) = "az";
  EXPECT_OK(repository.Update("gajanan", full_gajanan));
  ASSERT_EQ(repository.size(), 3);
  (*full_gajanan.mutable_user()->mutable_name()) = "gajanan";
  EXPECT_OK(repository.Update("az", full_gajanan));
  ASSERT_EQ(repository.size(), 3);
  (*full_gajanan.mutable_user()->mutable_name()) = "gajananc";
  EXPECT_OK(repository.Update("gajanan", full_gajanan));
  ASSERT_EQ(repository.size(), 3);
  (*full_gajanan.mutable_user()->mutable_name()) = "gajanan";
  EXPECT_OK(repository.Update("gajananc", full_gajanan));
  ASSERT_EQ(repository.size(), 3);
}

} // anonymous namespace
} // namespace acumio

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
