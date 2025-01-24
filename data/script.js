//Initilize tooltips boostrap
const tooltipTriggerList = document.querySelectorAll('[data-bs-toggle="tooltip"]');
const tooltipList = [...tooltipTriggerList].map(tooltipTriggerEl => new bootstrap.Tooltip(tooltipTriggerEl));

//Set up heading for firebase app
const database = firebase.database();
// Database Paths
const dataIPPath = 'status/ip';
const lastSessionUIDPath = 'status/lastSessionUID';
const totalUSerPath = 'status/totalUser'
// Get a database reference 
var databaseIP = database.ref(dataIPPath);
// Variables to save database current values
var ipReading;
var lastUIDReading;
var records,users;
var totalUserReading;
var isLastUIDRetrived = false;
// Attach an asynchronous callback to read the data
databaseIP.on('value', (snapshot) => {
    ipReading = snapshot.val();
    // document.getElementById("reading-ip").innerHTML = ipReading;
}, (errorObject) => {
  console.log('The read failed: ' + errorObject.name);
});

//SVG
var svgUsername = `
<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" fill="currentColor" class="bi bi-person-vcard" viewBox="0 0 16 16">
  <path d="M5 8a2 2 0 1 0 0-4 2 2 0 0 0 0 4m4-2.5a.5.5 0 0 1 .5-.5h4a.5.5 0 0 1 0 1h-4a.5.5 0 0 1-.5-.5M9 8a.5.5 0 0 1 .5-.5h4a.5.5 0 0 1 0 1h-4A.5.5 0 0 1 9 8m1 2.5a.5.5 0 0 1 .5-.5h3a.5.5 0 0 1 0 1h-3a.5.5 0 0 1-.5-.5"/>
  <path d="M2 2a2 2 0 0 0-2 2v8a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V4a2 2 0 0 0-2-2zM1 4a1 1 0 0 1 1-1h12a1 1 0 0 1 1 1v8a1 1 0 0 1-1 1H8.96q.04-.245.04-.5C9 10.567 7.21 9 5 9c-2.086 0-3.8 1.398-3.984 3.181A1 1 0 0 1 1 12z"/>
</svg>
`;

var svgUID =`
<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" fill="currentColor" class="bi bi-hash" viewBox="0 0 16 16">
  <path d="M8.39 12.648a1 1 0 0 0-.015.18c0 .305.21.508.5.508.266 0 .492-.172.555-.477l.554-2.703h1.204c.421 0 .617-.234.617-.547 0-.312-.188-.53-.617-.53h-.985l.516-2.524h1.265c.43 0 .618-.227.618-.547 0-.313-.188-.524-.618-.524h-1.046l.476-2.304a1 1 0 0 0 .016-.164.51.51 0 0 0-.516-.516.54.54 0 0 0-.539.43l-.523 2.554H7.617l.477-2.304c.008-.04.015-.118.015-.164a.51.51 0 0 0-.523-.516.54.54 0 0 0-.531.43L6.53 5.484H5.414c-.43 0-.617.22-.617.532s.187.539.617.539h.906l-.515 2.523H4.609c-.421 0-.609.219-.609.531s.188.547.61.547h.976l-.516 2.492c-.008.04-.015.125-.015.18 0 .305.21.508.5.508.265 0 .492-.172.554-.477l.555-2.703h2.242zm-1-6.109h2.266l-.515 2.563H6.859l.532-2.563z"/>
</svg>`;

var svgAge=`
<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" fill="currentColor" class="bi bi-cake2-fill" viewBox="0 0 16 16">
  <path d="m2.899.804.595-.792.598.79A.747.747 0 0 1 4 1.806v4.886q-.532-.09-1-.201V1.813a.747.747 0 0 1-.1-1.01ZM13 1.806v4.685a15 15 0 0 1-1 .201v-4.88a.747.747 0 0 1-.1-1.007l.595-.792.598.79A.746.746 0 0 1 13 1.806m-3 0a.746.746 0 0 0 .092-1.004l-.598-.79-.595.792A.747.747 0 0 0 9 1.813v5.17q.512-.02 1-.055zm-3 0v5.176q-.512-.018-1-.054V1.813a.747.747 0 0 1-.1-1.01l.595-.79.598.789A.747.747 0 0 1 7 1.806"/>
  <path d="M4.5 6.988V4.226a23 23 0 0 1 1-.114V7.16c0 .131.101.24.232.25l.231.017q.498.037 1.02.055l.258.01a.25.25 0 0 0 .26-.25V4.003a29 29 0 0 1 1 0V7.24a.25.25 0 0 0 .258.25l.259-.009q.52-.018 1.019-.055l.231-.017a.25.25 0 0 0 .232-.25V4.112q.518.047 1 .114v2.762a.25.25 0 0 0 .292.246l.291-.049q.547-.091 1.033-.208l.192-.046a.25.25 0 0 0 .192-.243V4.621c.672.184 1.251.409 1.677.678.415.261.823.655.823 1.2V13.5c0 .546-.408.94-.823 1.201-.44.278-1.043.51-1.745.696-1.41.376-3.33.603-5.432.603s-4.022-.227-5.432-.603c-.702-.187-1.305-.418-1.745-.696C.408 14.44 0 14.046 0 13.5v-7c0-.546.408-.94.823-1.201.426-.269 1.005-.494 1.677-.678v2.067c0 .116.08.216.192.243l.192.046q.486.116 1.033.208l.292.05a.25.25 0 0 0 .291-.247M1 8.82v1.659a1.935 1.935 0 0 0 2.298.43.935.935 0 0 1 1.08.175l.348.349a2 2 0 0 0 2.615.185l.059-.044a1 1 0 0 1 1.2 0l.06.044a2 2 0 0 0 2.613-.185l.348-.348a.94.94 0 0 1 1.082-.175c.781.39 1.718.208 2.297-.426V8.833l-.68.907a.94.94 0 0 1-1.17.276 1.94 1.94 0 0 0-2.236.363l-.348.348a1 1 0 0 1-1.307.092l-.06-.044a2 2 0 0 0-2.399 0l-.06.044a1 1 0 0 1-1.306-.092l-.35-.35a1.935 1.935 0 0 0-2.233-.362.935.935 0 0 1-1.168-.277z"/>
</svg>
`;

var svgTotalRecord =   `
<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" fill="currentColor" class="bi bi-search-heart" viewBox="0 0 16 16">
  <path d="M6.5 4.482c1.664-1.673 5.825 1.254 0 5.018-5.825-3.764-1.664-6.69 0-5.018"/>
  <path d="M13 6.5a6.47 6.47 0 0 1-1.258 3.844q.06.044.115.098l3.85 3.85a1 1 0 0 1-1.414 1.415l-3.85-3.85a1 1 0 0 1-.1-.115h.002A6.5 6.5 0 1 1 13 6.5M6.5 12a5.5 5.5 0 1 0 0-11 5.5 5.5 0 0 0 0 11"/>
</svg>
`;

var svgTotalUser =   `
<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" fill="currentColor" class="bi bi-people-fill" viewBox="0 0 16 16">
  <path d="M7 14s-1 0-1-1 1-4 5-4 5 3 5 4-1 1-1 1zm4-6a3 3 0 1 0 0-6 3 3 0 0 0 0 6m-5.784 6A2.24 2.24 0 0 1 5 13c0-1.355.68-2.75 1.936-3.72A6.3 6.3 0 0 0 5 9c-4 0-5 3-5 4s1 1 1 1zM4.5 8a2.5 2.5 0 1 0 0-5 2.5 2.5 0 0 0 0 5"/>
</svg>
`;


//Function to get data from firebase
async function getLastUID() {
  try {
    const snapshot = await database.ref(lastSessionUIDPath).once('value');
    lastUIDReading = snapshot.val();
    $("#currentUID").empty();
    $("#currentUID").append(svgUID);    
    $("#currentUID").append(lastUIDReading);
  } catch (error) {
    console.error('Lỗi khi lấy dữ liệu:', error);
  }
}


var noUserHTML = `
<button type="button" class="btn btn-sm btn-outline-secondary" > Không tìm thấy người dùng nào. Hãy tạo người dùng đầu tiên.                           
</button>
`; 

var currentUserNamePath;
var currentUserNameReading;
async function getcurrentUserName() {
  try {
    const snapshot = await database.ref('users/'+ lastUIDReading +'/username').once('value');
    currentUserNameReading = snapshot.val();
    $("#currentUsername").empty();
    $("#currentUsername").append(svgUsername);    
    $("#currentUsername").append(currentUserNameReading);
  } catch (error) {
    console.error('Lỗi khi lấy dữ liệu:', error);
  }
}

var currentUserAgeReading;
async function getcurrentUserAge() {
    try {
      const snapshot = await database.ref('users/'+ lastUIDReading +'/age').once('value');
      currentUserAgeReading = snapshot.val();
      $("#currentUserAge").empty();       
      $("#currentUserAge").append(svgAge);    
      $("#currentUserAge").append(currentUserAgeReading);
    } catch (error) {
      console.error('Lỗi khi lấy dữ liệu:', error);
    }
}


var currentTotalRecordReading;
async function getcurrentTotalRecord() {
    try { 
      const snapshot = await database.ref("users/" + lastUIDReading + "/recordID_last").once('value');
      currentTotalRecordReading = snapshot.val();
      if (currentTotalRecordReading === 0 ){
        $("#deleteRecordButton").attr("disabled", true);
      }else{
        $("#deleteRecordButton").attr("disabled", false);
      } 
      $("#currentTotalRecord").empty();       
      $("#currentTotalRecord").append(svgTotalRecord);    
      $("#currentTotalRecord").append(currentTotalRecordReading); 
    } catch (error) {
      console.error('Lỗi khi lấy dữ liệu:', error);
    }
}

var isUserSynced = false;

async function getTotalUser() {
    try {
      const snapshot = await database.ref(totalUSerPath).once('value');
      totalUserReading = snapshot.val();
      $("#currentTotalUser").empty();       
      $("#currentTotalUser").append(svgTotalUser);    
      $("#currentTotalUser").append(totalUserReading); 
      if (totalUserReading === 0 || totalUserReading == null){
        $("#userNavLink").addClass("disabled").attr("arrial-disabled", true);
        $("#recordNavLink").addClass("disabled").attr("arrial-disabled", true);

        $("#changeUIDField").attr("disabled", true);
        $("#changeUIDButton").attr("disabled", true);

        $("#deleteRecordButton").attr("disabled", true);
        $("#deleteAllButton").attr("disabled", true);

        $("#currentUID").empty().append("Không tìm thấy người dùng nào");
        // $("#currentUID").prop("data-bs-title", "lmao");
        $("#currentUsername").empty().append("Vui lòng tạo thêm");
        // $("#currentUsername").attr("data-bs-toggle", null);
        $("#currentTotalRecord").empty();
        // $("#currentTotalRecord").attr("data-bs-toggle", "disabled");
        // $(".btntooltip").prop("data-bs-title", "");
        const tooltip1 = bootstrap.Tooltip.getInstance('#currentUID');
        tooltip1.disable();
        const tooltip2 = bootstrap.Tooltip.getInstance('#currentUsername');
        tooltip2.disable();
        const tooltip3 = bootstrap.Tooltip.getInstance('#currentTotalRecord');
        tooltip3.disable();  
     }
     else if (totalUserReading === 1){
        $("#changeUIDField").attr("disabled", true);
        $("#changeUIDButton").attr("disabled", true);
        $(".btntooltip").attr("data-bs-toggle", "tooltip");
        $("#deleteRecordButton").attr("disabled", true);
        await getLastUID();
        await getcurrentUserName();
        await getcurrentTotalRecord();
     }
     else{
        $("#changeUIDField").attr("max", totalUserReading);
        $("#deleteRecordButton").attr("disabled", true);
        $(".btntooltip").attr("data-bs-toggle", "tooltip");
        await getLastUID();
        await getcurrentUserName();
        await getcurrentTotalRecord();
     }
    } catch (error) {
      console.error('Lỗi khi lấy dữ liệu:', error);
    }
}



//Fetching data to table
var recordTable;
var userTable;
async function fetchingAtCtrlTab() {
    await getTotalUser();

}

// var currentUserName ;
async function fetchRecord() {
  await getLastUID();
  await getcurrentUserName();
  await getcurrentUserAge();
  await getcurrentTotalRecord();
  try {
    const snapshot = await database.ref('record/'+ currentUserNameReading).once('value');
    records = snapshot.val();
    if(records === null){
      $("tbody-record").remove();

      recordTable = new $('#record-table').DataTable( {
        data: records,
      } );
    }
    else if (records === 1){
      recordDummy = null;
      recordTable = new $('#record-table').DataTable( {
        data: recordDummy,
      } );
    }
    else  {
     records = records.filter(item => item !== null);  
     recordTable = new $('#record-table').DataTable( {
      data: records,
      columns: [
        { data: 'id'},
        { data: 'BPM' },
        { data: 'spo2' },
        { data: 'analyzed' },
        { data: 'timestamp' },
      ] 
    } );
    }

  } catch (error) {
    console.error('Lỗi khi lấy dữ liệu:', error);
  }
}

async function fetchUser() { 
    await getLastUID();
    await getcurrentUserName();
    await getcurrentUserAge();
    await getcurrentTotalRecord();
    try {
      const snapshot = await database.ref('users/').once('value');
      users = snapshot.val();
      if(users === 1 || users === null){
        $("tbody-user").remove();
        users = null;
        userTable = new $('#user-table').DataTable( {
          data: users,
        } );
      }
      else{
       users = users.filter(item => item !== null);  
       userTable = new $('#user-table').DataTable( {
        data: users,
        columns: [
          { data: 'id'},
          { data: 'username' },
          { data: 'age' },
          { data: 'recordID_last' },
          { data: 'dateCreated'}
        ] 
      } );
      }
  
    } catch (error) {
      console.error('Lỗi khi lấy dữ liệu:', error);
    }
}

//Divide script.js into equivalent html 

//index.html
if ($("#controllerTab").length){
  $(document).ready(async function() {
    await fetchingAtCtrlTab();
  });  
  // $("#deleteRecordForm").submit(function(event) {
  //   event.preventDefault();
  // });
  // $("#deleteAllForm").submit(function(event) {
  //   event.preventDefault();
  // });
  $('#deleteAllButton').click(async function() {
      if (confirm('Bạn có chắc chắn muốn xóa toàn bộ dữ liệu? Hành động này không thể khôi phục.')) {
        $("#deleteAllButton").val("confirmed"); 
        await database.ref().remove()
        .then(() => {
          console.log('Dữ liệu đã được xóa thành công!');
        })
        .catch((error) => {
          console.error('Lỗi khi xóa dữ liệu:', error);
        });
        // location.reload(true);
 
      }
      else{
        $("#deleteAllButton").val("false"); 
      }

  });
  
  $('#deleteRecordButton').click(async function() {
      if (confirm('Bạn có chắc chắn muốn xóa dữ liệu đo của người dùng hiện tại? Hành động này không thể khôi phục.')) {
        var currentRecordPath = "record/" + currentUserNameReading;
        var currentRecordLastPath = "users/" + lastUIDReading;
        $("#deleteRecordButton").val("confirmed");
        await database.ref(currentRecordPath).remove()
        .then(() => {
          console.log('Dữ liệu đã được xóa thành công!');
        })
        .catch((error) => {
          console.error('Lỗi khi xóa dữ liệu:', error);
        });
        database.ref(currentRecordLastPath).update({
            recordID_last: 0
        })      
        .then(() => {
            console.log('Dữ liệu đã được cập nhật thành công!');
          })
        .catch((error) => {
            console.error('Lỗi khi cập nhật dữ liệu:', error);         
        });

          // location.reload(true);
      }
      else{
        $("#deleteRecordButton").val("false");
      }
  });
  $('#reloadController').click(async function () {
    await getTotalUser();
  });
}

//record.html
if ($("#recordTab").length){

    fetchRecord();
    $('#reloadRecord').click(async function () {
        recordTable.destroy();
        fetchRecord();
    });

}

//user.html
if ($("#userTab").length){
    
    fetchUser();
    $('#reloadUser').click(async function () {
        userTable.destroy();
        fetchUser();
    });


}



