package tools;

import java.io.InputStream;
import java.io.OutputStream;
import java.security.Key;
import java.security.PublicKey;
import java.security.PrivateKey;
import java.security.KeyPair;
import java.security.KeyPairGenerator;
import java.security.NoSuchAlgorithmException;
import java.security.InvalidKeyException;
import java.security.spec.X509EncodedKeySpec;
import java.security.spec.PKCS8EncodedKeySpec;
import java.security.KeyFactory;
 
import javax.crypto.Cipher;
import javax.crypto.CipherInputStream;
import javax.crypto.CipherOutputStream;
import javax.crypto.NoSuchPaddingException;
import javax.crypto.IllegalBlockSizeException;

import java.util.Base64;

public class Encryption{
  private KeyPair keys = null;
  private static String mode = "RSA";
  private Cipher encCipher = null;
  private Cipher decCipher = null;

  public static Encryption createInstance(int keyBits){
    try{
     KeyPairGenerator keygen = KeyPairGenerator.getInstance(mode);
     keygen.initialize(keyBits);
     KeyPair rsaKeys = keygen.genKeyPair();
     return new Encryption(rsaKeys);
    } catch(Exception e){
      return null;
    }
  }


  public static Encryption createInstance(String publicKey_PEM){
   try {
      publicKey_PEM = publicKey_PEM.replace("-----BEGIN PRIVATE KEY-----\n", "");
      publicKey_PEM = publicKey_PEM.replace("-----END PRIVATE KEY-----", "");
      byte[] encoded = Base64.getDecoder().decode(publicKey_PEM);
      KeyFactory kf = KeyFactory.getInstance("RSA");
      PKCS8EncodedKeySpec keySpec = new PKCS8EncodedKeySpec(encoded);
      PublicKey pubKey = kf.generatePublic(keySpec);
      return new Encryption(pubKey); 
   } catch(Exception e){
      return null;
   }
  }
  


  private Encryption(KeyPair kp) throws NoSuchAlgorithmException,NoSuchPaddingException,InvalidKeyException{
     init(kp);
  } 

 
  private Encryption(PublicKey k) throws NoSuchAlgorithmException,NoSuchPaddingException,InvalidKeyException{
     init(new KeyPair(k,null));
  } 

  private void init(KeyPair kp) throws NoSuchAlgorithmException, NoSuchPaddingException,InvalidKeyException{

    // TODO: seeding the used random number generator
    keys = kp;
    Key puk = kp.getPublic();
    if(puk!=null){
      encCipher = Cipher.getInstance(mode);
      encCipher.init(Cipher.ENCRYPT_MODE, kp.getPublic());
    }
    Key prk = kp.getPrivate();
    if(prk!=null){
      decCipher = Cipher.getInstance(mode);
      decCipher.init(Cipher.DECRYPT_MODE, kp.getPrivate());
    }
  }

  public byte[] encrypt(byte[] plain){
    try {
       return encCipher!=null?encCipher.doFinal(plain):null;
    } catch(Exception e){
      return null;
    }
  }

  public byte[] decrypt(byte[] crypted){
    try{
       return decCipher!=null?decCipher.doFinal(crypted):null;
    } catch(Exception e){
      return null;
    }
  }

  public PublicKey getPublicKey(){
    return keys.getPublic();
  }

  public String getPublicKey_PEM(){
    byte[] data = keys.getPublic().getEncoded();
    String base64encoded = new String(Base64.getEncoder().encode(data));
    return "-----BEGIN PUBLIC KEY-----\n" + base64encoded + "-----END PUBLIC KEY-----";
  }


  public static PublicKey generatePublicKey(byte[] bytes){
    try {
        return KeyFactory.getInstance(mode).generatePublic(new X509EncodedKeySpec(bytes));
    } catch(Exception e){
        return null;
    }
  }

  public static PrivateKey generatePrivateKey(byte[] bytes){
     try {
       return KeyFactory.getInstance(mode).generatePrivate(new PKCS8EncodedKeySpec(bytes));
    } catch(Exception e){
        return null;
    }
  }


}
