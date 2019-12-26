function doPost (e) {
  var folder, folders = DriveApp.getFoldersByName ("ESP32-CAM");
  if (folders.hasNext ()) {
    folder = folders.next ();
  } else {
    folder = DriveApp.createFolder ("ESP32-CAM");
  }

//  var filename_txt = Utilities.formatDate (new Date (), "GMT-3", "yyyyMMdd_HHmmss") + ".txt";
//  var blob_txt = Utilities.newBlob (JSON.stringify(e, null, 2), "text/plain", filename_txt);
//  folder.createFile (blob_txt);

  var data = Utilities.base64Decode (e.postData.contents);
  var filename = Utilities.formatDate (new Date (), "GMT-3", "yyyyMMdd_HHmmss") + ".jpg";
  var blob = Utilities.newBlob (data, "image/jpeg", filename);
  folder.createFile (blob);
 
  return ContentService.createTextOutput ("Complete.")
}