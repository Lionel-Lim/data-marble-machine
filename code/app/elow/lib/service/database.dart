import 'package:firebase_auth/firebase_auth.dart';
import 'package:firebase_database/firebase_database.dart';
import 'package:flutter/material.dart';

class DatabaseController {
  FirebaseAuth auth = FirebaseAuth.instance;
  FirebaseDatabase database = FirebaseDatabase.instance;
  final dbEntry = '/TEST';

  Future<List<dynamic>> getStructure() async {
    DatabaseReference dbRef = database.ref();
    final snapshot =
        await dbRef.child('$dbEntry/${auth.currentUser?.uid}').get();
    if (snapshot.exists) {
      Map<dynamic, dynamic>? data = snapshot.value as Map<dynamic, dynamic>?;
      if (data != null) {
        print(data);
        print("data is");
        print(traverseJson(data).toSet().toList());
      } else {
        return [];
      }
    } else {
      return [];
    }
    return [];
  }

  void getData({required String address}) async {
    DatabaseReference dbRef = database.ref();
    final snapshot =
        await dbRef.child('$address/${auth.currentUser?.uid}').get();
    if (snapshot.exists) {
      // print(snapshot.value);
    } else {
      debugPrint("No data");
    }
    // if (snapshot.exists) {
    //   Map<dynamic, dynamic>? data = snapshot.value as Map<dynamic, dynamic>?;
    //   if (data != null) {
    //     printAllKeys(data);
    //   } else {
    //     print('No data available');
    //   }
    // } else {
    //   print('No data available');
    // }
  }

  List<List<dynamic>> traverseJson(Map<dynamic, dynamic> value,
      [List<dynamic>? hierarchy]) {
    List<List<dynamic>> result = [];

    // Initialize hierarchy list if it's null
    hierarchy ??= [];

    // Create a new copy of the hierarchy list for this function call
    List<dynamic> newHierarchy = List.from(hierarchy);

    value.forEach((key, val) {
      newHierarchy.add(key);

      if (val is Map<dynamic, dynamic>) {
        result.addAll(traverseJson(val, newHierarchy));
      } else {
        newHierarchy.removeLast(); // Remove the key added for a leaf node
        result.add(newHierarchy);
      }

      newHierarchy
          .removeLast(); // Remove the last added key for the next iteration
    });

    return result;
  }
}
